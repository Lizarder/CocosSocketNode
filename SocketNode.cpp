#include "network/SocketIO.h"
#include "SocketNode.h"
#include "cocos2d.h"

USING_NS_CC;

class SocketNode::Impl
{
public:
	SocketNode* _socketNode;
	SOCKET  _socketImpl;
	static const int STATUS_OK = 0;
	static const int STATUS_STOPPED = 1;
	static const int STATUS_FINISHED = 2;

	bool stopCalled;
	bool inDestructor;
	Ref* pTarget;
	bool disconnectByClient;
public:
	Impl(SocketNode* _this)
	{
		this->_socketNode = _this;
		stopCalled = false;
		inDestructor = false;
		pTarget = nullptr;
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
		
	}

	void start(const char* host, int port)
	{
		connectionThread(host, port);
	}
	void* connectionThread(const char* host, int port)
	{
		struct sockaddr_in m_ServerAddr;
		SOCKET _socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (_socket == INVALID_SOCKET)
		{
			cocos2d::log("cocos2d debug--->:connect failed !");
			return NULL;
		}
		_socketImpl = _socket;
		memset(&m_ServerAddr, 0, sizeof(m_ServerAddr));
		m_ServerAddr.sin_family = AF_INET;
		m_ServerAddr.sin_port = htons(port); 

		bool dnsOk = false;

		if (strlen(host) > 0)
		{
			hostent* hptr = gethostbyname(host);
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
			cocos2d::log("cocos2d debug--->:connect failed !");
			return NULL;
		}

		cocos2d::log("cocos2d debug--->:connect successed !");
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
};


SocketNode::SocketNode()
{
	impl = new Impl(this);
}

SocketNode::~SocketNode()
{
	closeSocket();
	if (impl)
	{
		delete impl;
		impl = nullptr;
	}
}

SocketNode* SocketNode::create(const char* addr, int port, cocos2d::Ref* pTarget)
{
	SocketNode* _instance = new SocketNode();
	_instance->init();
	_instance->autorelease();
	_instance->impl->pTarget = pTarget;
	_instance->impl->start(addr, port);
	return _instance;
}

void SocketNode::stop()
{
	//unimplemented
}

void SocketNode::sendData(const char* pData, int size)
{
	//unimplemented
}

void SocketNode::update(float delta)
{
	//unimplemented
}

void SocketNode::closeSocket()
{
	this->impl->closeSocket(this->impl->_socketImpl);
}