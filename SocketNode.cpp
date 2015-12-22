#include "network/SocketIO.h"
#include "SocketNode.h"
#include "cocos2d.h"

USING_NS_CC;

class SocketNode::Impl
{
public:
	SocketNode* _socketNode;

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


private:

};


SocketNode::SocketNode(){

}