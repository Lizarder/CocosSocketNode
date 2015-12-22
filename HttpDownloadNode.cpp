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
			// ���������뻥����
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

			// ���������̲߳��������ݡ�
#if CC_TARGET_PLATFORM==CC_PLATFORM_WIN32
			DWORD tid = 0;
			CreateThread(NULL, 0, &Impl::downloadThrd, NULL, 0, &tid);
#else
			pthread_t thrd;
			pthread_create(&thrd, NULL, &Impl::downloadThrd, (void*)info);
			pthread_detach(thrd);
#endif
			// ����update���
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
			FILE				*f;				// buffer��f����ֻ��һ����Ч��
			unsigned int		savedSize;
			Info				*info;
			CURL				*curl;
			unsigned int		resumeIdx;
		};

		static size_t onDownloadWrite(char* ptr, size_t size, size_t nmemb, void *userdata)
		{
			DownloadData* pDD = (DownloadData*)userdata;
			// �״ν��յ�����ʱ֪ͨ�ļ���С��
			if (pDD->savedSize == 0)
			{
				double contentLen = -1;
				if (curl_easy_getinfo(pDD->curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &contentLen) == CURLE_OK)
				{
					// Ԥ�ȷ���ռ�
					pDD->buffer.reserve((unsigned int)contentLen);

					MUTEX_LOCK(pDD->info->mtx);
					void* data = malloc(4);
					unsigned int len = (unsigned int)(contentLen + pDD->resumeIdx);
					memcpy(data, &len, 4);
					pDD->info->notifies.push_back(Notify(HttpDownloadNode::NOTIFY_SIZE, data, 4));
					MUTEX_UNLOCK(pDD->info->mtx);
				}
			}
			// ���յ�����ʱ�����savePathNameΪ���򱣴����ڴ��У�������ӵ��ļ�ĩβ��
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
			// ��statusΪSTATUS_STOPPEDʱ���ж����ء�
			if (pDD->info->status == STATUS_STOPPED)
			{
				MUTEX_UNLOCK(pDD->info->mtx);
				return 0;
			}
			// ����֪ͨ
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
				// ���extractToFolder���ڣ����ѹ��ָ��Ŀ¼����ɾ��ԭѹ���ļ���
				if (!pInfo->savePathName.empty() && !pInfo->extractToFolder.empty())
				{
					UnzipToDirA(pInfo->savePathName.c_str(), pInfo->extractToFolder.c_str());
					remove(pInfo->savePathName.c_str());
				}
				// �����/����Ͷ�ݵ������в���status��ΪSTATUS_FINISHED�������߳�
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
			// �����bytes��savePathName����Ҫ���Ȼ�ȡ�������ļ��Ĵ�С��
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
					// ����ʱֱ���˳���
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
				resumeLast = false;		// ����Ϊfalse�������ͨ��������Ϊtrue
				// �����bytes������ȡ�����ļ���С������savePathName
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

						// ����savePathName+".info"�е��ļ���С�Ƿ�һ�¡�
						// �������md5����Ӧ���savePathName+".info"�е�md5�Ƿ�һ�¡�
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
				// ��һ�µĻ�������д�룻
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
			// ��ȡ�����ص��ļ�����
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
			// ��ȡ�ļ���Ϣ�������savePathName������ļ���
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
			// һ�µĻ�����Ҫ�趨CURLҪ�ϵ�������λ��
			if (resumeLast && resumeIdx > 0)
			{
				curl_easy_setopt(handle, CURLOPT_RESUME_FROM, resumeIdx);
			}
			// ��ʱʱ����Ϊһ�죬�������ز����û�Ӧ���ܹ�ȡ��
			curl_easy_setopt(handle, CURLOPT_TIMEOUT, 3600 * 24);
			// 
			curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, &onDownloadWrite);
			curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void*)&dd);
			curl_easy_setopt(handle, CURLOPT_USERAGENT, "libcurl");
			curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1L);
			curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
			char errorbuf[128];
			curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, errorbuf);

			// ʹ��curl�������ء�
			bool downloadOK = false, stopped = false;
			CURLcode curlCode;
			if ((curlCode = curl_easy_perform(handle)) == CURLE_OK)
			{
				downloadOK = true;
				curl_easy_cleanup(handle);
				// �ر��ļ���
				if (dd.f)
				{
					fflush(dd.f);
					fclose(dd.f);
					dd.f = NULL;
				}
			}
			else
			{
				// ����ʱֱ���˳���
				curl_easy_cleanup(handle);
				// �ر��ļ���
				if (dd.f)
				{
					fflush(dd.f);
					fclose(dd.f);
					dd.f = NULL;
				}
				// �����֧�ֶϵ���������ɾ��ԭ�ļ���
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
			// ��statusΪSTATUS_STOPPEDʱ�������߳�mutex�����������ݲ������̡߳�(����Info�ṹ��Ӧ�������߳����)
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
			// ������ϣ�
			if (downloadOK)
			{
				// ɾ��savePathName+".info"�ļ�
				if (!pInfo->savePathName.empty())
				{
					std::string infoFile = pInfo->savePathName + ".info";
					remove(infoFile.c_str());
				}
				// ���bytes���ڣ������С�Ƿ���ȷ��
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
						// ��С����ȷ�򱨸�����˳��̡߳�
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
				// ���md5Val���ڣ�����md5�Ƿ���ȷ��
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
						// MD5����ȷ�򱨸�����˳��߳�
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
				// ���extractToFolder���ڣ����ѹ��ָ��Ŀ¼����ɾ��ԭѹ���ļ���
				if (!pInfo->savePathName.empty() && !pInfo->extractToFolder.empty())
				{
					UnzipToDirA(pInfo->savePathName.c_str(), pInfo->extractToFolder.c_str());
					remove(pInfo->savePathName.c_str());
				}
				// �����/����Ͷ�ݵ������в���status��ΪSTATUS_FINISHED�������߳�
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
			// ���notifies���������¼���������ͬ�Ĳ�����
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
			// ���statusΪSTATUS_FINISHED����ֹͣ��
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
				// ֹͣupdate���
				_this->unscheduleUpdate();
				// ��status��ΪSTATUS_STOPPED���Ա��������߳�ֹͣ��
				MUTEX_LOCK(info->mtx);
				if (info->status == STATUS_FINISHED)
				{
					// ע�⣺��������߳��Ѿ���status��ΪSTATUS_FINISHED�ˣ�����Ҫ�����������߳�mutex������notifies��
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
				// ����������������У���Ӹ��ڵ��Ƴ���
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
