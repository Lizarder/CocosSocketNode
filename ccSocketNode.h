#ifndef __SOCKET_NODE_H__
#define __SOCKET_NODE_H__
#include "cocos2d.h"

namespace lk {

class SocketNode : public cocos2d::Node {
	class Impl;
	friend class Impl;
	Impl * impl;

	SocketNode();
	~SocketNode();
public:

	static const unsigned int NOTIFY_CONNECTED = 1;
	static const unsigned int NOTIFY_CONNECT_FAILED = 2;
	static const unsigned int NOTIFY_DISCONNECTED = 3;
	static const unsigned int NOTIFY_PACKET = 4;

	typedef void (cocos2d::Ref::*SEL_CallFuncUDU)(unsigned int, void*, unsigned int);

	// 通过TCP连接指定的服务器地址。
	// onNotify用以接收进度通知与结束通知: onNotify(eventType, data); 消息列表为：
	//     "connected", nil;
	//     "connectfailed", reason;
	//     "disconnected", reason;
	//     "packet", packetContent: string;
	static SocketNode* createLua(const char* addr, int port, int onNotify);

	static SocketNode* create(const char* addr, int port, cocos2d::Ref* pTarget, SEL_CallFuncUDU selector);

	void sendData(const char* data, int size);

	// 停止后会自动从父节点中移除自身。
	void stop();

	virtual void update(float delta);
};

};		// namespace lk

#endif // #ifndef __SOCKET_NODE_H__