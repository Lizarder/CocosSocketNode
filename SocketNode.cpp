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

USING_NS_CC;

class SocketNode::Impl
{
public:
	SocketNode* _socketNode;

	struct Notify{
		unsigned int type;
		void* data;
		unsigned int len;
		Notify(unsigned int type, void* data, unsigned int len)
		{
			this->type = type;
			this->data = data;
			this->len = len;
		}
		Notify(unsigned int type, char* data)
		{
			this->type = type;
			this->data = data;
			this->len = strlen(data);
		}
/*		~Notify()
		{
			if (data)
			{
				free(data);
				data = NULL;
			}
			
		}*/
	};
	static const int STATUS_OK = 0;
	static const int STATUS_STOPPED = 1;
	static const int STATUS_FINISHED = 2;
	struct WriteThreadData
	{
		MUTEX mtx; //�ź���
		std::deque<Notify> packages;
		unsigned int state;
		SOCKET _socket;
	};
	struct ReadThreadData
	{
		std::string addr;
		unsigned int port;
		MUTEX mtx;
		std::deque <Notify> notifies;
		unsigned int state;
		SOCKET _socket;
		WriteThreadData* wtd;
	};

	ReadThreadData* rtd;  // ���߳�
	WriteThreadData* wtd; //д�߳�

	bool stopCalled;
	bool inDestructor;
	bool disconnectByClient;
	Ref* pTarget;
	SocketNode::SEL_CallFuncUDU callfunc;
public:
	Impl(SocketNode* _this)
	{
		this->_socketNode = _this;
		rtd = NULL;
		wtd = NULL;
		stopCalled = false;
		inDestructor = false;
		pTarget = NULL;
		callfunc = NULL;
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

	void start(const char* addr, int port)
	{
		wtd = new WriteThreadData();
		MUTEX_INIT(wtd->mtx);
		wtd->state = STATUS_OK;
		wtd->_socket = INVALID_SOCKET;
		//����д�߳�
		{
#if CC_TARGET_PLATFORM==CC_PLATFORM_WIN32
			DWORD tid = 0;
			CreateThread(NULL, 0, &Impl::writeThread, (void*)wtd, 0, &tid);//����д�߳�
#else
			pthread_t _thread;
			pthread_create(&_thread, NULL, &Impl::writeThread, (void*)wtd);
			pthread_detach(_thread);
#endif
		}
		rtd = new ReadThreadData();
		rtd->addr = addr;
		rtd->port = port;
		rtd->wtd = wtd;
		rtd->state = STATUS_OK;
		MUTEX_INIT(rtd->mtx);
		rtd->_socket = INVALID_SOCKET;
		////�������߳�
		{
#if CC_TARGET_PLATFORM==CC_PLATFORM_WIN32
			DWORD tid = 0;
			CreateThread(NULL, 0, &Impl::connectionThread, (void*)rtd, 0, &tid);
#else
			pthread_t _thread;
			pthread_create(&_thread, NULL, &Impl::connectionThread, (void*)rtd);
			pthread_detach(_thread);
#endif
		}
		_socketNode->scheduleUpdate();
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
#if CC_TARGET_PLATFORM==CC_PLATFORM_WIN32
	static DWORD WINAPI connectionThread(LPVOID data)
#else
	static void* connectionThread(void* data)
#endif
	{
		cocos2d::log("cocos2d debug---->:connect thread start");
		cocos2d::ThreadHelper thread;
		thread.createAutoreleasePool();

		ReadThreadData* rtd = (ReadThreadData*)data;
		std::string host = rtd->addr;
		unsigned int port = rtd->port;

		struct sockaddr_in m_ServerAddr;
		//��������
		{
			SOCKET _socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (_socket == INVALID_SOCKET)
			{
				MUTEX_LOCK(rtd->mtx);
				rtd->notifies.push_back(Notify(SocketNode::NOTIFY_CONNECT_FAILED, mallocStrS("connect failed %s",host.c_str())));
				rtd->state = STATUS_FINISHED;
				MUTEX_UNLOCK(rtd->mtx);
				cocos2d::log("cocos2d debug--->:connect failed !");
				return NULL;
			}


			memset(&m_ServerAddr, 0, sizeof(m_ServerAddr));
			m_ServerAddr.sin_family = AF_INET;
			m_ServerAddr.sin_port = htons(port);

			bool dnsOk = false;

			if (!host.empty())
			{
				hostent* hptr = gethostbyname(host.c_str());
				if (hptr != NULL && hptr->h_addrtype == AF_INET)
				{
					dnsOk = true;
					memcpy(&m_ServerAddr.sin_addr, hptr->h_addr_list[0], sizeof(m_ServerAddr));
				}
			}
			if (!dnsOk)
			{
				cocos2d::log("cocos2d debug--->:connect failed dns wrong!");
				return NULL;
			}

			int iErrorCode = 0;
			iErrorCode = connect(_socket, (struct sockaddr*)&m_ServerAddr, sizeof(m_ServerAddr));
			if (iErrorCode == SOCKET_ERROR)
			{
				MUTEX_LOCK(rtd->mtx);
				rtd->notifies.push_back(Notify(SocketNode::NOTIFY_CONNECT_FAILED, mallocStrS("connect failed %s", host.c_str())));
				rtd->state = STATUS_FINISHED;
				MUTEX_UNLOCK(rtd->mtx);
				cocos2d::log("cocos2d debug--->:connect failed !");
				return NULL;
			}

			MUTEX_LOCK(rtd->mtx);
			rtd->_socket = _socket;
			rtd->notifies.push_back(Notify(SocketNode::NOTIFY_CONNECTED, mallocStrS("connect successful %s", host.c_str())));
			WriteThreadData* wtd = rtd->wtd;
			MUTEX_UNLOCK(rtd->mtx);
			MUTEX_LOCK(wtd->mtx);
			wtd->_socket = _socket;
			MUTEX_UNLOCK(wtd->mtx);
			cocos2d::log("cocos2d debug--->:connect successed !");
		}

		std::vector<char> buff(1024 * 16);//recv����
		int recvSize = 0;//�������ݴ�С
		while (true)
		{
			MUTEX_LOCK(rtd->mtx);
			//���statusΪSTATUS_STOPPED����ɾ�������˳�
			if (rtd->state == STATUS_STOPPED)
			{
				MUTEX_UNLOCK(rtd->mtx);
				//
				break;
			}
			else
			{
				MUTEX_UNLOCK(rtd->mtx);
			}
			//��ʼ��ȡ
			int recvCode = recv(rtd->_socket, &buff[0] + recvSize, buff.size(), 0);
			cocos2d::log("cocos2d debug recvdata: %d",recvCode);
			if (recvCode < 0)
			{
				//���ӶϿ�
				closeSocket(rtd->_socket);
				cocos2d::log("read thread close socket");
				//�����˳�
				MUTEX_LOCK(rtd->mtx);
				if (rtd->state == STATUS_STOPPED)
				{
					// ���statusΪSTATUS_STOPPED����ôɾ�����ݲ��˳���
					MUTEX_UNLOCK(rtd->mtx);
					// ����ѭ�����Ա�ɾ�����ݲ��˳���
					break;
				}
				else
				{
					rtd->state = STATUS_FINISHED;
					rtd->notifies.push_back(Notify(SocketNode::NOTIFY_DISCONNECTED, mallocStrS("disconnect  %s", host.c_str())));
					MUTEX_UNLOCK(rtd->mtx);
					cocos2d::log("cocos2d debug:----------->connection thread out");
					return NULL;
				}

			}

			recvSize += recvCode;

			char *readp = &buff[0];
			while (true)
			{
				//
				if (recvSize < 2)
				{
					break;
				}
				//��ȡ������
				unsigned int packLen = 0;
				memcpy(((char*)&packLen) + 1, readp, 1);
				memcpy((char*)&packLen, readp + 1, 1);

				if (recvSize < packLen + 2)
				{
					break;//��δ����
				}
				//Ͷ�������߳�
				char *copiedData = (char*)malloc(packLen);
				memcpy(copiedData, readp + 2, packLen);

				cocos2d::log("cocos2d debug recvdata: %s", copiedData);

				MUTEX_LOCK(rtd->mtx);
				rtd->notifies.push_back(Notify(SocketNode::NOTIFY_PACKET, copiedData, packLen));
				MUTEX_UNLOCK(rtd->mtx);
				//ָ����һ����
				readp = readp + 2 + packLen;
				recvSize = recvSize - 2 - packLen;
			}

			if (readp != &buff[0])
			{
				memmove(&buff[0], readp, recvSize);
			}

		}
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
		cocos2d::log("cocos2d debug----------------->:connect thread out!\n");
		return NULL;
	
	}
#if CC_TARGET_PLATFORM==CC_PLATFORM_WIN32
	static DWORD WINAPI writeThread(LPVOID data)
#else
	void* writeThread(void* data)
#endif
	{
		cocos2d::log("cocos2d debug------>:send data and write data thread in!");
		cocos2d::ThreadHelper thread;
		thread.createAutoreleasePool();

		WriteThreadData* wtd = (WriteThreadData*)data;

		while (true)
		{
			MUTEX_LOCK(wtd->mtx);
			if (wtd->state == STATUS_STOPPED)
			{
				MUTEX_UNLOCK(wtd->mtx);
				break;
			}

			if (wtd->_socket == INVALID_SOCKET)
			{
				MUTEX_UNLOCK(wtd->mtx);
				Sleep(16);
				continue;
			}

			if (!wtd->packages.empty())
			{
				std::deque<Notify> packages = wtd->packages;
				wtd->packages.clear();
				MUTEX_UNLOCK(wtd->mtx);

				while (!packages.empty())
				{
					Notify& packet = packages.front();
					if (packet.data)
					{
						int sentSize = 0;
						int err = 0;
						cocos2d::log("cocos2d debug socket senddata");
						cocos2d::log((char*)packet.data);
						while (true)
						{
							err = send(wtd->_socket, ((char*)packet.data) + sentSize, packet.len - sentSize, 0);
							if (err > 0)
							{
								sentSize += err;
								if (sentSize >= packet.len)
								{
									break;
								}
							}
							else
							{
								break;
							}
						}
						if (err <= 0)
						{
							//���ӶϿ����ر��߳� ɾ����ʱpackages����
							while (!packages.empty())
							{
								Notify& packet = packages.front();
								if (packet.data)
								{
									free(packet.data);
									packet.data = NULL;
								}
								packages.pop_front();
							}

							closeSocket(wtd->_socket);
							cocos2d::log("write thread close socket");

							MUTEX_LOCK(wtd->mtx);
							if (wtd->state == STATUS_STOPPED)
							{
								MUTEX_UNLOCK(wtd->mtx);
								break;
							}
							else
							{
								wtd->state = STATUS_FINISHED;
								MUTEX_UNLOCK(wtd->mtx);
								cocos2d::log("cocos2d debug---------->:write thread out !");
								return NULL;
							}
						}
						//ɾ����ʱpacket������
						free(packet.data);
						packet.data = NULL;

					}
					//�Ӱ�����packages��ɾ����ǰ�İ�
					packages.pop_front();
				}
				//�˳��߳�
				continue;
			}
			else
			{
				MUTEX_UNLOCK(wtd->mtx);
				Sleep(16);
				continue;
			}
		}
		//���wtd����
		if (!wtd->packages.empty())
		{
			Notify& packet = wtd->packages.front();
			if (packet.data)
			{
				free(packet.data);
				packet.data = NULL;
			}
			wtd->packages.pop_front();
		}

		delete wtd;
		wtd = NULL;
		cocos2d::log("cocos2d debug:----------->:write thread returned!");
		return NULL;

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
		cocos2d::log("cocos2d debug--->:close connection  !");
	}

	void stop()
	{
		if (!stopCalled)
		{
			stopCalled = true;

			_socketNode->unscheduleUpdate();
			//ֹͣ���߳�
			MUTEX_LOCK(rtd->mtx);
			if (rtd->state == STATUS_FINISHED)
			{
				MUTEX_UNLOCK(rtd->mtx);
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
				//״̬��Ϊstoped
				rtd->state = STATUS_STOPPED;
				MUTEX_UNLOCK(rtd->mtx);
			}
			//ֹͣд�߳�
			MUTEX_LOCK(wtd->mtx);
			if (wtd->state == STATUS_FINISHED)
			{
				MUTEX_UNLOCK(wtd->mtx);
				while (!wtd->packages.empty())
				{
					Notify& packet = wtd->packages.front();
					if (packet.data)
					{
						free(packet.data);
						packet.data = NULL;
					}
					wtd->packages.pop_front();
				}

				delete wtd;
				wtd = NULL;
			}
			else
			{
				wtd->state = STATUS_STOPPED;
				MUTEX_UNLOCK(wtd->mtx);
			}

			//û��������Ӹ��ڵ�ɾ��socketnode
			if (!inDestructor)
			{
				_socketNode->removeFromParentAndCleanup(true);
			}
		}
	}

	void onCheckNotifies(float dt)
	{
		bool isFinished = false;
		
		std::deque<Notify> tempNotifies;
		MUTEX_LOCK(rtd->mtx);
		isFinished = (rtd->state == STATUS_FINISHED);
		tempNotifies = rtd->notifies;
		rtd->notifies.clear();
		MUTEX_UNLOCK(rtd->mtx);

		while (!tempNotifies.empty())
		{
			
			Notify& notify = tempNotifies.front();

			if (pTarget&&callfunc)
			{
				(pTarget->*callfunc)(notify.type, notify.data, notify.len);
			}

			if (notify.data)
			{
				free(notify.data);
				notify.data = NULL;
			}
			tempNotifies.pop_front();
		}

		if (isFinished)
		{
			stop();
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
		impl = nullptr;
	}
}

SocketNode* SocketNode::create(const char* addr, int port, cocos2d::Ref* pTarget, SEL_CallFuncUDU selector)
{
	SocketNode* _instance = new SocketNode();
	_instance->init();
	_instance->autorelease();
	_instance->impl->pTarget = pTarget;
	_instance->impl->callfunc = selector;
	_instance->impl->start(addr, port);
	return _instance;
}

void SocketNode::stop()
{
	impl->stop();
}

void SocketNode::sendData(const char* pData, int size)
{
	if (!impl->stopCalled)
	{
		//char* copiedData = (char*)malloc(size+2);
		char* copiedData = (char*)malloc(size);
		unsigned short len = (unsigned short)size;
		unsigned int dataLen = size;
		//memcpy(copiedData + 1, ((char*)&len), 1);
		//memcpy(copiedData, ((char*)&len) + 1, 1);
		//memcpy(copiedData+2, pData, dataLen);
		memcpy(copiedData , pData, dataLen);
		cocos2d::log(copiedData+2);
		cocos2d::log(pData);
		MUTEX_LOCK(impl->wtd->mtx);
		impl->wtd->packages.push_back(Impl::Notify(NOTIFY_PACKET, copiedData, dataLen));
		MUTEX_UNLOCK(impl->wtd->mtx);
		cocos2d::log("cocos2d debug------->:socketnode senddata");

	}
}

void SocketNode::update(float delta)
{
	impl->onCheckNotifies(delta);
}