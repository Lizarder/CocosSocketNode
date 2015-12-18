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

	//	����socket����ʱ������������
	virtual void onConnect(cocos2d::network::SIOClient* client);
	//	�����յ�����ʱ������������
	virtual void onMessage(cocos2d::network::SIOClient* client, const std::string& data);
	//	��socket�ر�ʱ��������������
	virtual void onClose(cocos2d::network::SIOClient* client);
	//	�����Ӵ���ʱ������������
	virtual void onError(cocos2d::network::SIOClient* client, const std::string& data);

	SIOClient *_sioClient;
	void createSocket();
	void testevent(SIOClient *client, const std::string& data);
	void echotest(SIOClient *client, const std::string& data);
	void SIOClientClicked(Ref* Obj);

	//	�������������Ϣ
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
	//	��ʼ��TestWebSocket
	void initTestWebSocket();
	static Scene* scene();

	//	����WebSocket����ʱ�ᱻ����
	virtual void onOpen(WebSocket* ws);
	//	����������ʱ�ᱻ����
	virtual void onMessage(WebSocket* ws, const WebSocket::Data& data);
	//	���ر�WebSocket����ʱ�ᱻ����
	virtual void onClose(WebSocket* ws);
	//	�����ӳ��ִ���ʱ�ᱻ����
	virtual void onError(WebSocket* ws, const WebSocket::ErrorCode& error);

	void  onMenuClicked(Ref* pObj);

};
#endif /* defined(__TestNetWork__TestNetWork__) */

