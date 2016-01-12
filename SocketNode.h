#ifndef _SOCKET_NODE_H_
#define _SOCKET_NODE_H_

#include "cocos2d.h"

#define SOCKET_PACKAGE 2046

class SocketNode : public cocos2d::Node{
	class Impl;
	friend class Impl;
	Impl* impl;

	SocketNode();
	~SocketNode();

public:
	static const unsigned int NOTIFY_CONNECTED = 1;
	static const unsigned int NOTIFY_CONNECT_FAILED = 2;
	static const unsigned int NOTIFY_DISCONNECTED = 3;
	static const unsigned int NOTIFY_PACKET = 4;

	typedef void (cocos2d::Ref::*SEL_CallFuncUDU)(unsigned int, void*, unsigned int);

	static SocketNode* create(const char* addr, int port, cocos2d::Ref* pTarget, SEL_CallFuncUDU selector);

	void sendData(const char* pData,int size);

	void stop();

	void closeSocket();

	virtual void update(float delta);
};
#endif