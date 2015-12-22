#include "HttpDownloadNode.h"
#include <stdio.h>
#include <iostream>
#include <set>
#include "utils.h"
#include "base/allocator/CCAllocatorMacros.h"
#include "base/allocator/CCAllocatorMutex.h"
#include <deque>
#include "unzipdir.h"
#include "CCLuaStack.h"
#include "CCLuaEngine.h"
#include <algorithm>
#include "curl/curl.h"

#if CC_LUA_ENGINE_ENABLED > 0
extern "C" {
#include "lua.h"
}
#include "CCLuaEngine.h"
#endif

using namespace cocos2d;

namespace lk {

	class HttpDownloadNode::Impl {
	public:
		HttpDownloadNode* _this;

		struct Notify {
			unsigned int	type;
			void*			data;
			unsigned int	len;
			Notify(unsigned int	type, void* data, unsigned int len)
			{
				this->type = type;
				this->data = data;
				this->len = len;
			}
			Notify(unsigned int	type, char* data)
			{
				this->type = type;
				this->data = data;
				this->len = strlen(data);
			}
		};

		static const unsigned int STATUS_OK = 0;
		static const unsigned int STATUS_STOPPED = 1;
		static const unsigned int STATUS_FINISHED = 2;

		struct Info {
			std::string				url;
			std::string				savePathName;
			unsigned int			bytes;
			std::string				md5Val;
			std::string				extractToFolder;

			MUTEX					mtx;
			std::deque<Notify>		notifies;
			unsigned int			status;

			bool					noProgress;
		};

		Info* info;
		LuaRef					onNotify;
		bool					stopCalled;
		bool					inDestructor;
		Ref						* pTarget;
		HttpDownloadNode::SEL_CallFuncUDU		selector;
	public:
		Impl(HttpDownloadNode* _this)
		{
			this->_this = _this;
			info = NULL;
			stopCalled = false;
			inDestructor = false;
			pTarget = NULL;
			selector = NULL;
		}
		~Impl()
		{
			inDestructor = true;
			stop();
		}

		void start(const char* url, LUA_FUNCTION onNotify, const char* savePathName /*= NULL*/,
			unsigned int bytes /*= 0*/, const char* md5Val /*= NULL*/, const char* extractToFolder /*= NULL*/, bool noProgress)
		{
			// 构造数据与互斥量
			Info* i = new Info();
			i->url = url;
			this->onNotify.reset(onNotify);
			i->savePathName = savePathName;
			i->bytes = bytes;
			i->md5Val = md5Val;
			i->extractToFolder = extractToFolder;
			i->noProgress = noProgress;

			MUTEX_INIT_EX(i->mtx, mtx);
			i->status = STATUS_OK;

			info = i;

			// 启动下载线程并传递数据。
#if CC_TARGET_PLATFORM==CC_PLATFORM_WIN32
			DWORD tid = 0;
			CreateThread(NULL, 0, &Impl::downloadThrd, NULL, 0, &tid);
#else
			pthread_t thrd;
			pthread_create(&thrd, NULL, &Impl::downloadThrd, (void*)info);
			pthread_detach(thrd);
#endif
			// 启动update检查
			_this->scheduleUpdate();
		}

		static char* mallocStrS(const char* fmtstr, const char* s)
		{
			char* result = (char*)malloc(strlen(fmtstr) + strlen(s) + 1);
			sprintf(result, fmtstr, s);
			return result;
		}

		static char* mallocStrIIS(const char* fmtstr, int i1, int i2, const char* s)
		{
			char* result = (char*)malloc(strlen(fmtstr) + 10 + 10 + strlen(s) + 1);
			sprintf(result, fmtstr, i1, i2, s);
			return result;
		}

		static char* mallocStrSSS(const char* fmtstr, const char* s1, const char* s2, const char* s)
		{
			char* result = (char*)malloc(strlen(fmtstr) + strlen(s1) + strlen(s2) + strlen(s) + 1);
			sprintf(result, fmtstr, s1, s2, s);
			return result;
		}

		struct DownloadData {
			std::vector<char>	buffer;
			FILE				*f;				// buffer与f有且只有一个有效。
			unsigned int		savedSize;
			Info				*info;
			CURL				*curl;
			unsigned int		resumeIdx;
		};

		static size_t onDownloadWrite(char* ptr, size_t size, size_t nmemb, void *userdata)
		{
			DownloadData* pDD = (DownloadData*)userdata;
			// 首次接收到数据时通知文件大小。
			if (pDD->savedSize == 0)
			{
				double contentLen = -1;
				if (curl_easy_getinfo(pDD->curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &contentLen) == CURLE_OK)
				{
					// 预先分配空间
					pDD->buffer.reserve((unsigned int)contentLen);

					MUTEX_LOCK(pDD->info->mtx);
					void* data = malloc(4);
					unsigned int len = (unsigned int)(contentLen + pDD->resumeIdx);
					memcpy(data, &len, 4);
					pDD->info->notifies.push_back(Notify(HttpDownloadNode::NOTIFY_SIZE, data, 4));
					MUTEX_UNLOCK(pDD->info->mtx);
				}
			}
			// 接收到数据时，如果savePathName为空则保存在内存中，否则添加到文件末尾。
			if (pDD->f)
			{
				fwrite(ptr, size, nmemb, pDD->f);
			}
			else
			{
				pDD->buffer.insert(pDD->buffer.end(), ptr, ptr + size*nmemb);
			}
			pDD->savedSize += size*nmemb;

			MUTEX_UNLOCK(pDD->info->mtx);
			// 当status为STATUS_STOPPED时则中断下载。
			if (pDD->info->status == STATUS_STOPPED)
			{
				MUTEX_UNLOCK(pDD->info->mtx);
				return 0;
			}
			// 进度通知
			if (!pDD->info->noProgress)
			{
				void* data = malloc(4);
				unsigned int ss = pDD->savedSize + pDD->resumeIdx;
				memcpy(data, &ss, 4);
				pDD->info->notifies.push_back(Notify(HttpDownloadNode::NOTIFY_PROGRESS, data, 4));
			}
			MUTEX_UNLOCK(pDD->info->mtx);
			return size*nmemb;
		}

#if CC_TARGET_PLATFORM==CC_PLATFORM_WIN32
		static DWORD WINAPI downloadThrd(LPVOID data)
#else
		static void* downloadThrd(void* data)
#endif
		{
			printf("--------thread in\n");
			// create autorelease pool for iOS
			cocos2d::ThreadHelper thread;
			thread.createAutoreleasePool();

			Info* pInfo = (Info*)data;

			if (pInfo->url.empty() && !pInfo->savePathName.empty() && !pInfo->extractToFolder.empty())
			{
				// 如果extractToFolder存在，则解压到指定目录，并删除原压缩文件。
				if (!pInfo->savePathName.empty() && !pInfo->extractToFolder.empty())
				{
					UnzipToDirA(pInfo->savePathName.c_str(), pInfo->extractToFolder.c_str());
					remove(pInfo->savePathName.c_str());
				}
				// 将结果/缓存投递到队列中并将status设为STATUS_FINISHED并结束线程
			{
				MUTEX_LOCK(pInfo->mtx);
				pInfo->notifies.push_back(Notify(HttpDownloadNode::NOTIFY_SUCCEEDED, NULL, 0));
				pInfo->status = STATUS_FINISHED;
				MUTEX_UNLOCK(pInfo->mtx);
				printf("--------thread out\n");
			}
			return NULL;
			}

			bool resumeLast = false;
			double contentLen = -1;
			// 如果有bytes和savePathName则需要事先获取所下载文件的大小。
			if (pInfo->bytes > 0 && !pInfo->savePathName.empty())
			{
				CURL* handle = curl_easy_init();
				curl_easy_setopt(handle, CURLOPT_URL, pInfo->url.c_str());
				curl_easy_setopt(handle, CURLOPT_NOBODY, 1);
				curl_easy_setopt(handle, CURLOPT_TIMEOUT, 5);
				curl_easy_setopt(handle, CURLOPT_USERAGENT, "libcurl");
				curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1L);
				curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
				if (curl_easy_perform(handle) == CURLE_OK)
				{
					if (curl_easy_getinfo(handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &contentLen) == CURLE_OK)
					{
						if (contentLen != -1 && contentLen == pInfo->bytes)
						{
							resumeLast = true;
						}
					}
					curl_easy_cleanup(handle);
				}
				else
				{
					// 出错时直接退出。
					curl_easy_cleanup(handle);
					MUTEX_LOCK(pInfo->mtx);
					pInfo->notifies.push_back(Notify(HttpDownloadNode::NOTIFY_FAILED,
						mallocStrS("Network error: Unable to get size via http: %s]", pInfo->url.c_str())));
					pInfo->status = STATUS_FINISHED;
					MUTEX_UNLOCK(pInfo->mtx);
					printf("--------thread out\n");
					return NULL;
				}
			}
			if (resumeLast)
			{
				resumeLast = false;		// 先设为false，待检查通过后再设为true
				// 如果有bytes，并获取到了文件大小，且有savePathName
				std::string infoFile = pInfo->savePathName + ".info";
				FILE* f = lk::fopenUtf8(infoFile.c_str(), "rb");
				if (f)
				{
					fseek(f, 0, SEEK_END);
					long infoLen = ftell(f);
					if (infoLen == 36)
					{
						fseek(f, 0, SEEK_SET);
						char savedMD5[33];
						savedMD5[32] = 0;
						fread(savedMD5, 1, 32, f);
						unsigned int savedLen = 0;
						fread(&savedLen, 1, 4, f);

						// 则检查savePathName+".info"中的文件大小是否一致。
						// 如果还有md5，则还应检查savePathName+".info"中的md5是否一致。
						if (savedLen == pInfo->bytes)
						{
							if (pInfo->md5Val.empty() || pInfo->md5Val == savedMD5)
							{
								resumeLast = true;
							}
						}
					}
					fclose(f);
				}
				// 不一致的话则重新写入；
				if (!resumeLast)
				{
					FILE* f = lk::fopenUtf8(infoFile.c_str(), "wb");
					if (f)
					{
						char savedMD5[33] = { 0 };
						if (pInfo->md5Val.size() == 32)
						{
							memcpy(savedMD5, pInfo->md5Val.c_str(), 32);
						}
						fwrite(savedMD5, 1, 32, f);
						unsigned int savedLen = pInfo->bytes;
						fwrite(&savedLen, 1, 4, f);
						fclose(f);
					}
				}
			}
			// 获取已下载的文件长度
			long resumeIdx = 0;
			if (resumeLast)
			{
				FILE* f = lk::fopenUtf8(pInfo->savePathName.c_str(), "rb");
				if (f)
				{
					fseek(f, 0, SEEK_END);
					resumeIdx = ftell(f);
					fclose(f);
				}
			}

		{
			DownloadData dd;
			dd.resumeIdx = resumeIdx;
			dd.f = NULL;
			dd.savedSize = 0;
			dd.info = pInfo;
			// 获取文件信息。如果有savePathName，则打开文件。
			if (!pInfo->savePathName.empty())
			{
				dd.f = lk::fopenUtf8(pInfo->savePathName.c_str(), resumeLast ? "ab" : "wb");
				if (!dd.f)
				{
					MUTEX_LOCK(pInfo->mtx);
					pInfo->notifies.push_back(Notify(HttpDownloadNode::NOTIFY_FAILED,
						mallocStrS("File error: Unable to open file to save: %s", pInfo->savePathName.c_str())));
					pInfo->status = STATUS_FINISHED;
					MUTEX_UNLOCK(pInfo->mtx);
					printf("--------thread out\n");
					return NULL;
				}
			}

			CURL* handle = curl_easy_init();
			dd.curl = handle;
			curl_easy_setopt(handle, CURLOPT_URL, pInfo->url.c_str());
			// 一致的话则需要设定CURL要断点续传的位置
			if (resumeLast && resumeIdx > 0)
			{
				curl_easy_setopt(handle, CURLOPT_RESUME_FROM, resumeIdx);
			}
			// 超时时间设为一天，反正下载不了用户应该能够取消
			curl_easy_setopt(handle, CURLOPT_TIMEOUT, 3600 * 24);
			// 
			curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, &onDownloadWrite);
			curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void*)&dd);
			curl_easy_setopt(handle, CURLOPT_USERAGENT, "libcurl");
			curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1L);
			curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
			char errorbuf[128];
			curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, errorbuf);

			// 使用curl启动下载。
			bool downloadOK = false, stopped = false;
			CURLcode curlCode;
			if ((curlCode = curl_easy_perform(handle)) == CURLE_OK)
			{
				downloadOK = true;
				curl_easy_cleanup(handle);
				// 关闭文件。
				if (dd.f)
				{
					fflush(dd.f);
					fclose(dd.f);
					dd.f = NULL;
				}
			}
			else
			{
				// 出错时直接退出。
				curl_easy_cleanup(handle);
				// 关闭文件。
				if (dd.f)
				{
					fflush(dd.f);
					fclose(dd.f);
					dd.f = NULL;
				}
				// 如果不支持断点续传，则删除原文件。
				if (curlCode == CURLE_RANGE_ERROR)
				{
					remove(pInfo->savePathName.c_str());
				}
				MUTEX_LOCK(pInfo->mtx);
				if (pInfo->status == STATUS_STOPPED)
				{
					stopped = true;
					MUTEX_UNLOCK(pInfo->mtx);
				}
				else
				{
					pInfo->notifies.push_back(Notify(HttpDownloadNode::NOTIFY_FAILED,
						mallocStrS(curlCode == CURLE_RANGE_ERROR ? "Network error: Unable to resume: %s" : "Network error: Unable to download: %s",
						pInfo->url.c_str())));
					pInfo->status = STATUS_FINISHED;
					MUTEX_UNLOCK(pInfo->mtx);
					printf("--------thread out\n");
					return NULL;
				}
			}
			// 当status为STATUS_STOPPED时则清理线程mutex、缓存与数据并结束线程。(否则Info结构体应该由主线程清除)
			if (stopped)
			{
				MUTEX_DESTROY(pInfo->mtx);
				while (!pInfo->notifies.empty())
				{
					Notify& notify = pInfo->notifies.front();
					if (notify.data)
					{
						free(notify.data);
						notify.data = NULL;
					}
					pInfo->notifies.pop_front();
				}
				delete pInfo;
				printf("--------thread out\n");
				return NULL;
			}
			// 接收完毕：
			if (downloadOK)
			{
				// 删除savePathName+".info"文件
				if (!pInfo->savePathName.empty())
				{
					std::string infoFile = pInfo->savePathName + ".info";
					remove(infoFile.c_str());
				}
				// 如果bytes存在，则检查大小是否正确。
				long downloadedSize = -1;
				if (!pInfo->savePathName.empty())
				{
					FILE* f = lk::fopenUtf8(pInfo->savePathName.c_str(), "rb");
					if (f)
					{
						fseek(f, 0, SEEK_END);
						downloadedSize = ftell(f);
						fclose(f);
					}
				}
				else
				{
					downloadedSize = dd.buffer.size();
				}

				if (pInfo->bytes > 0)
				{
					if (pInfo->bytes != downloadedSize)
					{
						// 大小不正确则报告错误并退出线程。
						MUTEX_LOCK(pInfo->mtx);
						pInfo->notifies.push_back(Notify(HttpDownloadNode::NOTIFY_FAILED,
							mallocStrIIS("Check error: size mismatch, %i downloaded but %i required for %s",
							(int)downloadedSize, pInfo->bytes, pInfo->url.c_str())));
						pInfo->status = STATUS_FINISHED;
						MUTEX_UNLOCK(pInfo->mtx);
						printf("--------thread out\n");
						return NULL;
					}
				}
				// 如果md5Val存在，则检查md5是否正确。
				if ((!pInfo->md5Val.empty()) && downloadedSize > 0)
				{
					std::string downloadedMD5;
					if (!pInfo->savePathName.empty())
					{
						std::vector<char> downloadedBytes(downloadedSize);
						FILE* f = lk::fopenUtf8(pInfo->savePathName.c_str(), "rb");
						if (f)
						{
							fread(&downloadedBytes[0], 1, downloadedSize, f);
							fclose(f);
						}
						downloadedMD5 = lk::Utils::md5(&downloadedBytes[0], downloadedSize);
					}
					else
					{
						downloadedMD5 = lk::Utils::md5(&dd.buffer[0], dd.buffer.size());
					}
					std::transform(downloadedMD5.begin(), downloadedMD5.end(), downloadedMD5.begin(), ::tolower);
					std::transform(pInfo->md5Val.begin(), pInfo->md5Val.end(), pInfo->md5Val.begin(), ::tolower);
					if (downloadedMD5 != pInfo->md5Val)
					{
						// MD5不正确则报告错误并退出线程
						MUTEX_LOCK(pInfo->mtx);
						pInfo->notifies.push_back(Notify(HttpDownloadNode::NOTIFY_FAILED,
							mallocStrSSS("Check error: MD5 mismatch, %s downloaded but %s required for %s",
							downloadedMD5.c_str(), pInfo->md5Val.c_str(), pInfo->url.c_str())));
						pInfo->status = STATUS_FINISHED;
						MUTEX_UNLOCK(pInfo->mtx);
						printf("--------thread out\n");
						return NULL;
					}
				}
				// 如果extractToFolder存在，则解压到指定目录，并删除原压缩文件。
				if (!pInfo->savePathName.empty() && !pInfo->extractToFolder.empty())
				{
					UnzipToDirA(pInfo->savePathName.c_str(), pInfo->extractToFolder.c_str());
					remove(pInfo->savePathName.c_str());
				}
				// 将结果/缓存投递到队列中并将status设为STATUS_FINISHED并结束线程
				{
					MUTEX_LOCK(pInfo->mtx);
					char* data = NULL;
					unsigned int dataSize = 0;
					if (!dd.buffer.empty())
					{
						dataSize = dd.buffer.size();
						data = (char*)malloc(dataSize);
						memcpy(data, &dd.buffer[0], dataSize);
					}
					pInfo->notifies.push_back(Notify(HttpDownloadNode::NOTIFY_SUCCEEDED, data, dataSize));
					pInfo->status = STATUS_FINISHED;
					MUTEX_UNLOCK(pInfo->mtx);
					printf("--------thread out\n");
					return NULL;
				}
			}
			printf("--------thread out\n");
			return NULL;
		}

		printf("--------thread out\n");
		return NULL;
		}

		void notifyLua(unsigned int type, void* data, unsigned int size)
		{
			LUA_FUNCTION handler = onNotify.getT();
			if (handler)
			{
				LuaStack* pStack = LuaEngine::getInstance()->getLuaStack();
				switch (type)
				{
				case HttpDownloadNode::NOTIFY_CONNECTED:
					lua_pushstring(pStack->getLuaState(), "connected");
					pStack->executeFunctionByHandler(handler, 1);
					break;
				case HttpDownloadNode::NOTIFY_SIZE:
					lua_pushstring(pStack->getLuaState(), "size");
					lua_pushnumber(pStack->getLuaState(), *(unsigned int*)data);
					pStack->executeFunctionByHandler(handler, 2);
					break;
				case HttpDownloadNode::NOTIFY_FILENAME:
					lua_pushstring(pStack->getLuaState(), "filename");
					lua_pushstring(pStack->getLuaState(), (const char*)data);
					pStack->executeFunctionByHandler(handler, 2);
					break;
				case HttpDownloadNode::NOTIFY_PROGRESS:
					lua_pushstring(pStack->getLuaState(), "progress");
					lua_pushnumber(pStack->getLuaState(), *(unsigned int*)data);
					pStack->executeFunctionByHandler(handler, 2);
					break;
				case HttpDownloadNode::NOTIFY_FAILED:
					lua_pushstring(pStack->getLuaState(), "failed");
					lua_pushstring(pStack->getLuaState(), (const char*)data);
					pStack->executeFunctionByHandler(handler, 2);
					break;
				case HttpDownloadNode::NOTIFY_SUCCEEDED:
					lua_pushstring(pStack->getLuaState(), "succeeded");
					if (data)
					{
						lua_pushlstring(pStack->getLuaState(), (const char*)data, size);
						pStack->executeFunctionByHandler(handler, 2);
					}
					else
					{
						pStack->executeFunctionByHandler(handler, 1);
					}
					break;
				}
			}
		}

		void onCheckNotifies(float dt)
		{
			// 检查notifies，并根据事件类型做不同的操作。
			bool isFinished = false;
			std::deque<Notify> tmpNotifies;
			MUTEX_LOCK(info->mtx);
			isFinished = (info->status == STATUS_FINISHED);
			tmpNotifies = info->notifies;
			info->notifies.clear();
			MUTEX_UNLOCK(info->mtx);

			while (!tmpNotifies.empty())
			{
				Notify& notify = tmpNotifies.front();

				notifyLua(notify.type, notify.data, notify.len);
				if (pTarget&&selector)
				{
					(pTarget->*selector)(notify.type, notify.data, notify.len);
				}

				if (notify.data)
				{
					free(notify.data);
					notify.data = NULL;
				}
				tmpNotifies.pop_front();
			}
			// 如果status为STATUS_FINISHED，则停止。
			if (isFinished)
			{
				stop();
			}
		}

		void stop()
		{
			if (!stopCalled)
			{
				stopCalled = true;
				onNotify.reset();
				// 停止update检查
				_this->unscheduleUpdate();
				// 将status设为STATUS_STOPPED，以便让下载线程停止。
				MUTEX_LOCK(info->mtx);
				if (info->status == STATUS_FINISHED)
				{
					// 注意：如果下载线程已经将status设为STATUS_FINISHED了，则需要在这里清理线程mutex与所有notifies。
					MUTEX_UNLOCK(info->mtx);
					MUTEX_DESTROY(info->mtx);
					while (!info->notifies.empty())
					{
						Notify& notify = info->notifies.front();
						if (notify.data)
						{
							free(notify.data);
							notify.data = NULL;
						}
						info->notifies.pop_front();
					}
					delete info;
					info = NULL;
				}
				else
				{
					info->status = STATUS_STOPPED;
					MUTEX_UNLOCK(info->mtx);
				}
				// 如果不在析构函数中，则从父节点移除。
				if (!inDestructor)
				{
					_this->removeFromParentAndCleanup(true);
				}
			}
		}
	};

	HttpDownloadNode::HttpDownloadNode()
	{
		impl = new Impl(this);
	}

	HttpDownloadNode::~HttpDownloadNode()
	{
		if (impl)
		{
			delete impl;
			impl = NULL;
		}
	}

	void HttpDownloadNode::stop()
	{
		impl->stop();
	}

	void HttpDownloadNode::update(float delta)
	{
		impl->onCheckNotifies(delta);
	}

	HttpDownloadNode* HttpDownloadNode::createLua(const char* url, int onNotify, const char* savePathName /*= ""*/, unsigned int bytes /*= 0*/, const char* md5Val /*= ""*/, const char* extractToFolder /*= ""*/, bool noProgress /*= false*/)
	{
		HttpDownloadNode* result = new HttpDownloadNode();
		result->init();
		result->autorelease();
		result->impl->start(url, onNotify, savePathName, bytes, md5Val, extractToFolder, noProgress);
		return result;
	}

	HttpDownloadNode* HttpDownloadNode::create(const char* url, cocos2d::Ref* pTarget, SEL_CallFuncUDU selector,
		const char* savePathName, unsigned int bytes,
		const char* md5Val, const char* extractToFolder, bool noProgress)
	{
		HttpDownloadNode* result = new HttpDownloadNode();
		result->init();
		result->autorelease();
		result->impl->pTarget = pTarget;
		result->impl->selector = selector;
		result->impl->start(url, NULL, savePathName, bytes, md5Val, extractToFolder, noProgress);
		return result;
	}

};// namespace lk
