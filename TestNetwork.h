//
//  TestNetWork.h
//  TestNetWork
//
//  Created by www.sydev.cn on 14-10-8.
//
//

#ifndef __TestNetWork__TestNetWork__
#define __TestNetWork__TestNetWork__

#include <stdio.h>
#include "cocos2d.h"

//	http
#include "network/HttpClient.h"
#include "network/HttpRequest.h"
#include "network/HttpResponse.h"

using namespace cocos2d::network;
using namespace cocos2d;

class TestNetWork : public cocos2d::Layer
{
public:
	static cocos2d::Scene* createScene();
	virtual bool init();
	CREATE_FUNC(TestNetWork);

	Size size;

	void createButton();
	void menuClickCallback(cocos2d::Ref* pSender);

	//	GET
	void createGetHttp();
	void getHttp_handshakeResponse(HttpClient *sender, HttpResponse *response);
	//	POST
	void createPostHttp();
	void PostHttp_handshakeResponse(HttpClient *sender, HttpResponse *response);
	//	Socket
	void createSocket();
	void Socket_handshakeResponse(HttpClient *sender, HttpResponse *response);
};

//	socket
#include "extensions/cocos-ext.h"
#include "network/SocketIO.h"
class TestSocket
	:public Layer
	, public SocketIO::SIODelegate
{
public:

	void initLayer();
	static Scene* scene();

	//	当打开socket连接时会调用这个函数
	virtual void onConnect(cocos2d::network::SIOClient* client);
	//	当接收到数据时会调用这个函数
	virtual void onMessage(cocos2d::network::SIOClient* client, const std::string& data);
	//	当socket关闭时，会调用这个函数
	virtual void onClose(cocos2d::network::SIOClient* client);
	//	当连接错误时会调用这个函数
	virtual void onError(cocos2d::network::SIOClient* client, const std::string& data);

	SIOClient *_sioClient;
	void createSocket();
	void testevent(SIOClient *client, const std::string& data);
	void echotest(SIOClient *client, const std::string& data);
	void SIOClientClicked(Ref* Obj);

	//	向服务器发送消息
	void sendMessage();
	void sendEvent();
	void closeSocket();
};


#include "network/WebSocket.h"

class TestWebSocket
	: public Layer
	, public WebSocket::Delegate
{
	WebSocket* _wsiSendText, *_wsiSendBinary, *_wsiError;
	int  _sendTextTimes, _sendBinaryTimes;
public:
	//	初始化TestWebSocket
	void initTestWebSocket();
	static Scene* scene();

	//	当打开WebSocket连接时会被调用
	virtual void onOpen(WebSocket* ws);
	//	当返回数据时会被调用
	virtual void onMessage(WebSocket* ws, const WebSocket::Data& data);
	//	当关闭WebSocket连接时会被调用
	virtual void onClose(WebSocket* ws);
	//	当连接出现错误时会被调用
	virtual void onError(WebSocket* ws, const WebSocket::ErrorCode& error);

	void  onMenuClicked(Ref* pObj);

};
#endif /* defined(__TestNetWork__TestNetWork__) */

