#include "SocketNode.h"
#include <stdio.h>
#include <iostream>
#include <set>
#include "base/allocator/CCAllocatorMacros.h"
#include "base/allocator/CCAllocatorMutex.h"
#include <deque>

#include <algorithm>

#if CC_TARGET_PLATFORM!=CC_PLATFORM_WIN32
#include <string.h>

#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include <errno.h>
#include <unistd.h>
#include <stdarg.h>

#define SOCKET int
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)
#endif

#if CC_LUA_ENGINE_ENABLED > 0
extern "C" {
#include "lua.h"
}
#include "CCLuaEngine.h"
#endif

using namespace cocos2d;

namespace lk {

	class SocketNode::Impl {
	public:
		SocketNode* _this;

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

		struct WriteThreadData {
			MUTEX					mtx;
			std::deque<Notify>		packets;
			unsigned int			status;
			SOCKET					skt;
		};

		struct ReadThreadData {
			std::string				addr;
			int						port;

			MUTEX					mtx;
			std::deque<Notify>		notifies;
			unsigned int			status;
			SOCKET					skt;
			WriteThreadData			*wtd;
		};

		ReadThreadData			*rtd;
		WriteThreadData			*wtd;
		LuaRef					onNotify;
		bool					stopCalled;
		bool					inDestructor;
		Ref						*pTarget;
		SocketNode::SEL_CallFuncUDU		selector;
		bool					disconnectByClient;
	public:
		Impl(SocketNode* _this)
		{
			this->_this = _this;
			rtd = NULL;
			wtd = NULL;
			stopCalled = false;
			inDestructor = false;
			pTarget = NULL;
			selector = NULL;
			disconnectByClient = false;

#if CC_TARGET_PLATFORM==CC_PLATFORM_WIN32
			static bool wsaStarted = false;
			if (!wsaStarted)
			{
				wsaStarted = true;
				WSADATA Ws;
				if (WSAStartup(MAKEWORD(2, 2), &Ws) != 0)
					cocos2d::log("Init Windows Socket Failed");
			}
#endif
		}
		~Impl()
		{
			inDestructor = true;
			stop();
		}

		void start(const char* addr, int port, LUA_FUNCTION onNotify)
		{
			// 构造数据与互斥量
			this->onNotify.reset(onNotify);

			wtd = new WriteThreadData();
			MUTEX_INIT_EX(wtd->mtx, wtdmtx);
			wtd->status = STATUS_OK;
			wtd->skt = INVALID_SOCKET;

			// 启动写线程。
			{
#if CC_TARGET_PLATFORM==CC_PLATFORM_WIN32
			DWORD tid = 0;
			CreateThread(NULL, 0, &Impl::writeThrd, (void*)wtd, 0, &tid);
#else
			pthread_t thrd;
			pthread_create(&thrd, NULL, &Impl::writeThrd, (void*)wtd);
			pthread_detach(thrd);
#endif
			}

			rtd = new ReadThreadData();
			rtd->addr = addr;
			rtd->port = port;
			MUTEX_INIT_EX(rtd->mtx, rtdmtx);
			rtd->status = STATUS_OK;
			rtd->skt = INVALID_SOCKET;
			rtd->wtd = wtd;

			// 启动连接与读线程。
			{
#if CC_TARGET_PLATFORM==CC_PLATFORM_WIN32
			DWORD tid = 0;
			CreateThread(NULL, 0, &Impl::connectionThrd, (void*)rtd, 0, &tid);
#else
			pthread_t thrd;
			pthread_create(&thrd, NULL, &Impl::connectionThrd, (void*)rtd);
			pthread_detach(thrd);
#endif
			}

			// 启动update检查
			_this->scheduleUpdate();
		}

		static char* mallocStrS(const char* fmtstr, const char* s)
		{
			char* result = (char*)malloc(strlen(fmtstr) + strlen(s) + 1);
			sprintf(result, fmtstr, s);
			return result;
		}

		static char* mallocStrI(const char* fmtstr, int i)
		{
			char* result = (char*)malloc(strlen(fmtstr) + 12 + 1);
			sprintf(result, fmtstr, i);
			return result;
		}

		static char* mallocStrISS(const char* fmtstr, int i1, const char* s1, const char* s2)
		{
			char* result = (char*)malloc(strlen(fmtstr) + 10 + strlen(s1) + strlen(s2) + 1);
			sprintf(result, fmtstr, i1, s1, s2);
			return result;
		}

		static char* mallocStrSSS(const char* fmtstr, const char* s1, const char* s2, const char* s)
		{
			char* result = (char*)malloc(strlen(fmtstr) + strlen(s1) + strlen(s2) + strlen(s) + 1);
			sprintf(result, fmtstr, s1, s2, s);
			return result;
		}

		static void closeSocket(SOCKET skt)
		{
#if CC_TARGET_PLATFORM==CC_PLATFORM_WIN32
			shutdown(skt, SD_BOTH);
			closesocket(skt);
#else
			shutdown(skt, SHUT_RDWR);
			close(skt);
#endif
		}

#if CC_TARGET_PLATFORM==CC_PLATFORM_WIN32
		static DWORD WINAPI connectionThrd(LPVOID data)
#else
		static void* connectionThrd(void* data)
#endif
		{
			cocos2d::log("--------connection thread in\n");
			// create autorelease pool for iOS
			cocos2d::ThreadHelper thread;
			thread.createAutoreleasePool();

			ReadThreadData* rtd = (ReadThreadData*)data;
			std::string hostname = rtd->addr;
			int port = rtd->port;
			struct sockaddr_in m_ServerAddr;
			// 启动连接。
			{
				// 创建socket
				SOCKET skt = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				if (skt==INVALID_SOCKET)
				{
					// 连接失败：发起回调并退出线程。
					MUTEX_LOCK(rtd->mtx);
					rtd->notifies.push_back(Notify(SocketNode::NOTIFY_CONNECT_FAILED,
						mallocStrI("Create socket failed: errno = %d", errno)));
					rtd->status = STATUS_FINISHED;
					MUTEX_UNLOCK(rtd->mtx);
					cocos2d::log("--------connection thread out\n");
					return NULL;
				}

				// 解析主机
				memset(&m_ServerAddr, 0, sizeof(m_ServerAddr));
				m_ServerAddr.sin_family = AF_INET;
				m_ServerAddr.sin_port = htons(port);

				bool dnsOK = false;
				if (!hostname.empty())
				{
					hostent *hptr;
					if ((hptr = gethostbyname(hostname.c_str())) != NULL && hptr->h_addrtype == AF_INET)
					{
						dnsOK = true;
						memcpy(&m_ServerAddr.sin_addr, hptr->h_addr_list[0], sizeof(m_ServerAddr.sin_addr));
					}
				}

				if (!dnsOK)
				{
					// 解析失败：发起回调并退出线程。
					MUTEX_LOCK(rtd->mtx);
					rtd->notifies.push_back(Notify(SocketNode::NOTIFY_CONNECT_FAILED,
						mallocStrS("gethostbyname failed: addr = \"%s\"", hostname.c_str())));
					rtd->status = STATUS_FINISHED;
					MUTEX_UNLOCK(rtd->mtx);
					cocos2d::log("--------connection thread out\n");
					return NULL;
				}

				// 连接主机
				int iErrorCode = 0;
				iErrorCode = connect(skt, (struct sockaddr*)&m_ServerAddr, sizeof(m_ServerAddr));
				if (iErrorCode==SOCKET_ERROR)
				{
					// 连接失败：发起回调并退出线程。
					MUTEX_LOCK(rtd->mtx);
					rtd->notifies.push_back(Notify(SocketNode::NOTIFY_CONNECT_FAILED,
						mallocStrISS("connect failed: errono = %i, addr = \"%s\", ip = %s",
						errno, hostname.c_str(), inet_ntoa(m_ServerAddr.sin_addr))));
					rtd->status = STATUS_FINISHED;
					MUTEX_UNLOCK(rtd->mtx);
					cocos2d::log("--------connection thread out\n");
					return NULL;
				}

				// 连接成功：发起回调，进行接收操作，并启动写线程。
				MUTEX_LOCK(rtd->mtx);
				rtd->skt = skt;
				rtd->notifies.push_back(Notify(SocketNode::NOTIFY_CONNECTED, NULL, 0));
				WriteThreadData* wtd = rtd->wtd;
				MUTEX_UNLOCK(rtd->mtx);
				MUTEX_LOCK(wtd->mtx);
				wtd->skt = skt;
				MUTEX_UNLOCK(wtd->mtx);
			}

			std::vector<char> buff(1024*16);
			int recvSize = 0;
			while (true)
			{
				MUTEX_LOCK(rtd->mtx);
				// 如果status为STATUS_STOPPED，那么删除数据并退出。
				if (rtd->status == STATUS_STOPPED)
				{
					MUTEX_UNLOCK(rtd->mtx);
					// 跳出循环，以便删除数据并退出。
					break;
				}
				else
				{
					MUTEX_UNLOCK(rtd->mtx);
				}

				// 尝试读取。
				int iRetCode = recv(rtd->skt, &buff[0] + recvSize, buff.size() - recvSize, 0);
				if (iRetCode <= 0)
				{
					// 如果连接已断开，那么发起回调并关闭线程。
					//删除tmpPackets的数据
					closeSocket(rtd->skt);
					// 尝试退出
					MUTEX_LOCK(rtd->mtx);
					if (rtd->status == STATUS_STOPPED)
					{
						// 如果status为STATUS_STOPPED，那么删除数据并退出。
						MUTEX_UNLOCK(rtd->mtx);
						// 跳出循环，以便删除数据并退出。
						break;
					}
					else
					{
						// 否则发起回调并设为STATUS_FINISHED然后退出，让主线程删除数据
						rtd->status = STATUS_FINISHED;
						rtd->notifies.push_back(Notify(SocketNode::NOTIFY_DISCONNECTED,
							mallocStrISS("recv failed: errono = %i, addr = \"%s\", ip = %s",
							errno, hostname.c_str(), inet_ntoa(m_ServerAddr.sin_addr))));
						MUTEX_UNLOCK(rtd->mtx);
						cocos2d::log("--------connection thread out\n");
						return NULL;
					}
				}

				// 读取成功：判断是否已经有完整的数据包
				recvSize += iRetCode;

				char *readp = &buff[0];
				while (true)
				{
					// 检查包长度ushort
					if (recvSize < 2)
					{
						break;
					}
					unsigned short pktlen = 0;
					memcpy(((char*)&pktlen)+1, readp, 1);
					memcpy(((char*)&pktlen), readp + 1, 1);
					// 检查此包有没有读完
					if (recvSize < pktlen + 2)
					{
						break;
					}
					// 包已读完：将此包投送到主线程
					char* copiedData = (char*)malloc(pktlen);
					memcpy(copiedData, readp+2, pktlen);
					MUTEX_LOCK(rtd->mtx);
					rtd->notifies.push_back(Notify(SocketNode::NOTIFY_PACKET,
						copiedData, pktlen));
					MUTEX_UNLOCK(rtd->mtx);

					// 指向下一个包开头
					readp = readp + 2 + pktlen;
					recvSize = recvSize - 2 - pktlen;
				}
				// 将剩余的未读完的包移动到缓冲区头部
				if (readp != &buff[0])
				{
					memmove(&buff[0], readp, recvSize);
				}
			}

			// 删除数据并退出。
			MUTEX_DESTROY(rtd->mtx);
			while (!rtd->notifies.empty())
			{
				Notify& notify = rtd->notifies.front();
				if (notify.data)
				{
					free(notify.data);
					notify.data = NULL;
				}
				rtd->notifies.pop_front();
			}
			delete rtd;
			rtd = NULL;
			cocos2d::log("--------connection thread out\n");
			return NULL;
		}

		// 写线程
#if CC_TARGET_PLATFORM==CC_PLATFORM_WIN32
		static DWORD WINAPI writeThrd(LPVOID data)
#else
		static void* writeThrd(void* data)
#endif
		{
			cocos2d::log("--------write thread in\n");
			// create autorelease pool for iOS
			cocos2d::ThreadHelper thread;
			thread.createAutoreleasePool();

			WriteThreadData* wtd = (WriteThreadData*)data;
			// 轮询数据并尝试发送。
			while (true)
			{
				MUTEX_LOCK(wtd->mtx);
				if (wtd->status == STATUS_STOPPED)
				{
					// 如果status为STATUS_STOPPED，那么删除数据并退出。
					MUTEX_UNLOCK(wtd->mtx);
					// 跳出循环，以便删除数据并退出。
					break;
				}
				// 还未连接上：继续等待。
				if (wtd->skt == INVALID_SOCKET)
				{
					MUTEX_UNLOCK(wtd->mtx);
					SLEEP_MILLISEC(16);
					continue;
				}
				// 一切正常：查看有无数据需要发送，有则发送；
				if (!wtd->packets.empty())
				{
					std::deque<Notify> tmpPackets = wtd->packets;
					wtd->packets.clear();
					MUTEX_UNLOCK(wtd->mtx);

					while (!tmpPackets.empty())
					{
						Notify& packet = tmpPackets.front();
						if (packet.data)
						{
							// 完整地发送此包的数据。
							int sentSize = 0;
							int err = 0;
							do {
								err = send(wtd->skt, ((char*)packet.data) + sentSize, packet.len - sentSize, 0);
								if (err > 0)
								{
									sentSize = sentSize + err;
									if (sentSize >= packet.len)
									{
										break;
									}
								}
								else
								{
									break;
								}
							} while (true);
							if (err <= 0)
							{
								// 如果连接已断开，那么直接关闭线程，不用回调，反正连接线程会回调。
								//删除tmpPackets的数据
								while (!tmpPackets.empty())
								{
									Notify& packet = tmpPackets.front();
									if (packet.data)
									{
										free(packet.data);
										packet.data = NULL;
									}
									tmpPackets.pop_front();
								}
								closeSocket(wtd->skt);
								// 尝试退出
								MUTEX_LOCK(wtd->mtx);
								if (wtd->status == STATUS_STOPPED)
								{
									// 如果status为STATUS_STOPPED，那么删除数据并退出。
									MUTEX_UNLOCK(wtd->mtx);
									// 跳出循环，以便删除数据并退出。
									break;
								}
								else
								{
									// 否则设为STATUS_FINISHED并直接退出，让主线程删除数据
									wtd->status = STATUS_FINISHED;
									MUTEX_UNLOCK(wtd->mtx);
									cocos2d::log("--------write thread out\n");
									return NULL;
								}
							}

							free(packet.data);
							packet.data = NULL;
						}
						tmpPackets.pop_front();
					}
					continue;
				}
				else
				{
					// 无则等待。
					MUTEX_UNLOCK(wtd->mtx);
					SLEEP_MILLISEC(16);
					continue;
				}
			}

			// 删除数据并退出。
			MUTEX_DESTROY(wtd->mtx);
			while (!wtd->packets.empty())
			{
				Notify& packet = wtd->packets.front();
				if (packet.data)
				{
					free(packet.data);
					packet.data = NULL;
				}
				wtd->packets.pop_front();
			}
			delete wtd;
			wtd = NULL;
			cocos2d::log("--------write thread out\n");
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
				case SocketNode::NOTIFY_CONNECTED:
					lua_pushstring(pStack->getLuaState(), "connected");
					pStack->executeFunctionByHandler(handler, 1);
					break;
				case SocketNode::NOTIFY_CONNECT_FAILED:
					lua_pushstring(pStack->getLuaState(), "connectfailed");
					lua_pushlstring(pStack->getLuaState(), (char*)data, size);
					pStack->executeFunctionByHandler(handler, 2);
					break;
				case SocketNode::NOTIFY_DISCONNECTED:
				{
					lua_pushstring(pStack->getLuaState(), "disconnected");
					lua_pushlstring(pStack->getLuaState(), (char*)data, size);
					pStack->executeFunctionByHandler(handler, 2);
				}
					break;
				case SocketNode::NOTIFY_PACKET:
					lua_pushstring(pStack->getLuaState(), "packet");
					lua_pushlstring(pStack->getLuaState(), (const char*)data, size);
					pStack->executeFunctionByHandler(handler, 2);
					break;
				}
			}
		}

		void onCheckNotifies(float dt)
		{
			// 检查notifies，并根据事件类型做不同的操作。
			bool isFinished = false;
			std::deque<Notify> tmpNotifies;
			MUTEX_LOCK(rtd->mtx);
			isFinished = (rtd->status == STATUS_FINISHED);
			tmpNotifies = rtd->notifies;
			rtd->notifies.clear();
			MUTEX_UNLOCK(rtd->mtx);

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
				// 将status设为STATUS_STOPPED，以便让连接线程停止。
				MUTEX_LOCK(rtd->mtx);
				if (rtd->status == STATUS_FINISHED)
				{
					// 注意：如果下载线程已经将status设为STATUS_FINISHED了，则需要在这里清理线程mutex与所有notifies。
					MUTEX_UNLOCK(rtd->mtx);
					MUTEX_DESTROY(rtd->mtx);
					while (!rtd->notifies.empty())
					{
						Notify& notify = rtd->notifies.front();
						if (notify.data)
						{
							free(notify.data);
							notify.data = NULL;
						}
						rtd->notifies.pop_front();
					}
					delete rtd;
					rtd = NULL;
				}
				else
				{
					// 否则将status设为STOPPED，以让连接线程清除数据。
					rtd->status = STATUS_STOPPED;
					MUTEX_UNLOCK(rtd->mtx);
				}
				// 将status设为STATUS_STOPPED，以便让写线程停止。
				MUTEX_LOCK(wtd->mtx);
				if (wtd->status == STATUS_FINISHED)
				{
					// 注意：如果下载线程已经将status设为STATUS_FINISHED了，则需要在这里清理线程mutex与所有notifies。
					MUTEX_UNLOCK(wtd->mtx);
					MUTEX_DESTROY(wtd->mtx);
					while (!wtd->packets.empty())
					{
						Notify& packet = wtd->packets.front();
						if (packet.data)
						{
							free(packet.data);
							packet.data = NULL;
						}
						wtd->packets.pop_front();
					}
					delete wtd;
					wtd = NULL;
				}
				else
				{
					// 否则将status设为STOPPED，以让写线程清除数据。
					wtd->status = STATUS_STOPPED;
					MUTEX_UNLOCK(wtd->mtx);
				}
				// 如果不在析构函数中，则从父节点移除。
				if (!inDestructor)
				{
					_this->removeFromParentAndCleanup(true);
				}
			}
		}
	};

	SocketNode::SocketNode()
	{
		impl = new Impl(this);
	}

	SocketNode::~SocketNode()
	{
		if (impl)
		{
			delete impl;
			impl = NULL;
		}
	}

	void SocketNode::stop()
	{
		impl->stop();
	}

	void SocketNode::update(float delta)
	{
		impl->onCheckNotifies(delta);
	}

	void SocketNode::sendData(const char* data, int size)
	{
		if (!impl->stopCalled)
		{
			char* copiedData = (char*)malloc(size+2);
			unsigned short len = (unsigned short)size;
			memcpy(copiedData+1, ((char*)&len), 1);
			memcpy(copiedData, ((char*)&len)+1, 1);
			memcpy(copiedData+2, data, size);
			MUTEX_LOCK(impl->wtd->mtx);
			impl->wtd->packets.push_back(Impl::Notify(NOTIFY_PACKET, copiedData, size+2));
			MUTEX_UNLOCK(impl->wtd->mtx);
		}
	}


	SocketNode* SocketNode::createLua(const char* addr, int port, int onNotify)
	{
		SocketNode* result = new SocketNode();
		result->init();
		result->autorelease();
		result->impl->start(addr, port, onNotify);
		return result;
	}

	SocketNode* SocketNode::create(const char* addr, int port, cocos2d::Ref* pTarget, SEL_CallFuncUDU selector)
	{
		SocketNode* result = new SocketNode();
		result->init();
		result->autorelease();
		result->impl->pTarget = pTarget;
		result->impl->selector = selector;
		result->impl->start(addr, port, NULL);
		return result;
	}

};// namespace lk
