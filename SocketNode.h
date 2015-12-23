#ifndef _SOCKET_NODE_H_
#define _SOCKET_NODE_H_

#include "cocos2d.h"

class SocketNode : public cocos2d::Node{
	class Impl;
	friend class Impl;
	Impl* impl;

	SocketNode();
	~SocketNode();

public:
	
	static SocketNode* create(const char* addr, int port, cocos2d::Ref* pTarget);

	void sendData(const char* pData,int size);

	void stop();

	void closeSocket();

	virtual void update(float delta);
};
#endif