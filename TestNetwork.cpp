#include "TestNetWork.h"
#include "LoginScene.h"
enum Tag{
	ONE = 1,
	TWO,
	THREE,
	FOUR,
	FIVE
};

Scene* TestNetWork::createScene()
{
	auto scene = Scene::create();
	auto layer = TestNetWork::create();
	scene->addChild(layer);
	return scene;
}

bool TestNetWork::init()
{
	if (!Layer::init())
	{
		return false;
	}

	size = Director::getInstance()->getVisibleSize();

	this->createButton();

	return true;
}

//	������ť
void TestNetWork::createButton()
{
	//	GET ����ť
	auto getItem = MenuItemFont::create("TestNetWork --HTTP GET", CC_CALLBACK_1(TestNetWork::menuClickCallback, this));
	getItem->setPosition(Vec2(size.width / 2, size.height / 2 + 140));
	getItem->setTag(ONE);

	//	POST ����ť
	auto postItem = MenuItemFont::create("TestNetWork --HTTP POST", CC_CALLBACK_1(TestNetWork::menuClickCallback, this));
	postItem->setPosition(Vec2(size.width / 2, size.height / 2 + 70));
	postItem->setTag(TWO);

	//	Socket ����ť
	auto socketItem = MenuItemFont::create("TestNetWork -- Socket", CC_CALLBACK_1(TestNetWork::menuClickCallback, this));
	socketItem->setPosition(Vec2(size.width / 2, size.height / 2));
	socketItem->setTag(THREE);

	//	Socket ����ť
	auto webSocketItem = MenuItemFont::create("TestNetWork -- WebSocket", CC_CALLBACK_1(TestNetWork::menuClickCallback, this));
	webSocketItem->setPosition(Vec2(size.width / 2, size.height / 2 - 70));
	webSocketItem->setTag(FOUR);

	//	����
	auto backItem = MenuItemFont::create("Back", CC_CALLBACK_1(TestNetWork::menuClickCallback, this));
	backItem->setPosition(Vec2(size.width*0.9, 50));

	//	�����˵�
	auto menu = Menu::create(getItem, postItem, socketItem, webSocketItem, backItem, NULL);
	menu->setPosition(Vec2::ZERO);
	this->addChild(menu, 1);
}

//	��ť��Ӧ����
void TestNetWork::menuClickCallback(Ref* pSender)
{
	int tag = ((MenuItem*)pSender)->getTag();

	switch (tag)
	{
	case ONE:
		this->createGetHttp();
		break;
	case TWO:
		this->createPostHttp();
		break;
	case THREE:
		Director::getInstance()->replaceScene(TestSocket::scene());
		break;
	case FOUR:
		Director::getInstance()->replaceScene(TestWebSocket::scene());
		break;
	default:
		Director::getInstance()->replaceScene(LoginScene::createScene());
		break;
	}
}

//	GETͨ��
void TestNetWork::createGetHttp()
{
	//	����HttpRequest����
	auto request = new HttpRequest();
	//	��������
	request->setUrl("http://www.baidu.com");
	//	��������ʽ  GET����
	request->setRequestType(HttpRequest::Type::GET);
	//	����������ɺ�Ļص�����
	request->setResponseCallback(CC_CALLBACK_2(TestNetWork::getHttp_handshakeResponse, this));
	//	��������tag
	request->setTag("getHttp_handshake");
	//	����HttpClient���󣬲��ҷ�������
	HttpClient::getInstance()->send(request);
	//	�ͷ�HttpRequest����
	request->release();
}
void TestNetWork::getHttp_handshakeResponse(HttpClient *sender, HttpResponse *response)
{
	//	ͨ��response ��ȡ����tag���ܻ�ȡ��˵�����󷢳��������������tagֵ
	if (0 != strlen(response->getHttpRequest()->getTag()))
	{
		CCLOG("request tag: %s", response->getHttpRequest()->getTag());
	}

	//	�ж��Ƿ�����ɹ� ����һ: ��ȡ����״̬��
	long codeActive = response->getResponseCode();
	if (codeActive == 200)
	{
		MessageBox("�����ͳɹ�", "��ʾ");
		CCLOG("%ld", codeActive);
	}
	else
	{
		//	response->getErrorBuffer() ���ش������
		auto erroeStr = String::createWithFormat("������ʧ��! \n �������: %s", response->getErrorBuffer());
		MessageBox(erroeStr->getCString(), "��ʾ");
		CCLOG("%ld", codeActive);
	}

	//	�ж��Ƿ�����ɹ� ������:ʹ��HttpResponse�����ṩ�ĺ���isSucceed() �����ж�(����ֵbool����)
	if (!response->isSucceed())
	{
		//	response->getErrorBuffer() ���ش������
		auto erroeStr = String::createWithFormat("������ʧ��! \n �������: %s", response->getErrorBuffer());
		MessageBox(erroeStr->getCString(), "��ʾ");
		return;
	}

	//	ʹ��response->getResponseData(); ��ȡ���󷵻ص�����
	std::vector<char> *buffer = response->getResponseData();
	std::stringstream s;

	//	�������
	for (unsigned int i = 0; i < buffer->size(); i++)
	{
		s << (*buffer)[i];
	}
	CCLOG("data: %s", s.str().c_str());
}

//	POSTͨ��
void TestNetWork::createPostHttp()
{
	//	�� testCpp�п����ҵ�Դ��
	//	����HttpRequest����
	HttpRequest* request = new HttpRequest();
	//	��������
	request->setUrl("http://localhost:8080");
	//	��������ʽ  GET����
	request->setRequestType(HttpRequest::Type::POST);
	//	ѡ���Ե�����Http�����Content-Type
	std::vector<std::string> headers;
	headers.push_back("Content-Type: application/json; charset=utf-8");
	request->setHeaders(headers);
	//	����������ɺ�Ļص�����
	request->setResponseCallback(CC_CALLBACK_2(TestNetWork::PostHttp_handshakeResponse, this));

	// ʹ��setRequestData()���������ύ����
	const char* postData = "visitor=cocos2d&TestSuite=Extensions Test/NetworkTest";
	request->setRequestData(postData, strlen(postData));
	//	��������tag
	request->setTag("postHttp_handshake");
	//	����HttpClient���󣬲��ҷ�������
	HttpClient::getInstance()->send(request);
	//	�ͷ�HttpRequest����
	request->release();
}
void TestNetWork::PostHttp_handshakeResponse(HttpClient *sender, HttpResponse *response)
{
	//	ͨ��response ��ȡ����tag���ܻ�ȡ��˵�����󷢳��������������tagֵ
	if (0 != strlen(response->getHttpRequest()->getTag()))
	{
		CCLOG("request tag: %s", response->getHttpRequest()->getTag());
	}

	//	������
	long statusCode = response->getResponseCode();
	CCLOG("response code: %ld", statusCode);

	//	�ж������Ƿ�ɹ�
	if (!response->isSucceed())
	{
		//	response->getErrorBuffer() ���ش������
		auto erroeStr = String::createWithFormat("������ʧ��! \n �������: %s", response->getErrorBuffer());
		MessageBox(erroeStr->getCString(), "��ʾ");
		return;
	}

	// ������ص�����
	std::vector<char> *buffer = response->getResponseData();
	printf("Http Test, dump data: ");
	for (unsigned int i = 0; i < buffer->size(); i++)
	{
		printf("%c", (*buffer)[i]);
	}
}



/////////////////////////////////////////////////////// Socket /////////////////////////////////////////////////
void TestSocket::initLayer()
{
	auto size = Director::getInstance()->getVisibleSize();

	auto menuRequest = Menu::create();
	menuRequest->setPosition(Vec2::ZERO);
	addChild(menuRequest);

	auto itemSIOClient = MenuItemFont::create("1 Open SocketIO Client", CC_CALLBACK_1(TestSocket::SIOClientClicked, this));
	itemSIOClient->setPosition(Vec2(size.width / 2, size.height / 2 + 100));
	itemSIOClient->setTag(ONE);
	menuRequest->addChild(itemSIOClient);

	auto sendSIOClient = MenuItemFont::create("2 send Massage", CC_CALLBACK_1(TestSocket::SIOClientClicked, this));
	sendSIOClient->setPosition(Vec2(size.width / 2, size.height / 2 + 50));
	sendSIOClient->setTag(TWO);
	menuRequest->addChild(sendSIOClient);

	auto submitSIOClient = MenuItemFont::create("3 submit Massage", CC_CALLBACK_1(TestSocket::SIOClientClicked, this));
	submitSIOClient->setPosition(Vec2(size.width / 2, size.height / 2));
	submitSIOClient->setTag(THREE);
	menuRequest->addChild(submitSIOClient);

	auto closeSIOClient = MenuItemFont::create("4 close SocketIO Client", CC_CALLBACK_1(TestSocket::SIOClientClicked, this));
	closeSIOClient->setPosition(Vec2(size.width / 2, size.height / 2 - 50));
	closeSIOClient->setTag(FOUR);
	menuRequest->addChild(closeSIOClient);

	//	����
	auto backItem = MenuItemFont::create("Back", CC_CALLBACK_1(TestSocket::SIOClientClicked, this));
	backItem->setPosition(Vec2(size.width*0.9, 50));
	menuRequest->addChild(backItem);

}
Scene* TestSocket::scene()
{
	auto scene = Scene::create();
	auto layer = new TestSocket();
	layer->initLayer();
	scene->addChild(layer);

	return scene;
}

void TestSocket::SIOClientClicked(Ref* Obj)
{
	int tag = ((MenuItem*)Obj)->getTag();
	switch (tag) {
	case ONE:
		this->createSocket();
		break;
	case TWO:
		this->sendMessage();
		break;
	case THREE:
		this->sendEvent();
		break;
	case FOUR:
		this->closeSocket();
		break;
	default:
		Director::getInstance()->replaceScene(TestNetWork::createScene());
		break;
	}
}
//	��Socket����
void TestSocket::createSocket()
{
	//	 ��Socket����
	_sioClient = SocketIO::connect("ws://channon.us:3000", *this);
	//	���ñ������
	_sioClient->setTag("Test Client");

	//	ע��socket.io�¼��ص�����
	_sioClient->on("testevent", CC_CALLBACK_2(TestSocket::testevent, this));
	_sioClient->on("echotest", CC_CALLBACK_2(TestSocket::echotest, this));
}
//	�������������Ϣ
void TestSocket::sendMessage()
{
	if (_sioClient) {
		//	������Ϣ��socket.Io������
		_sioClient->send("Hello Socket.IO!");
	}
}
//	ί�д���socket.Io���¼�������emit����echotest�¼�
void TestSocket::sendEvent()
{
	if (_sioClient) {
		//	ί�д���socket.Io���¼�������emit����echotest�¼�
		_sioClient->emit("echotest", "[{\"name\":\"myname\",\"type\":\"mytype\"}]");
	}
}
//	�ر�socket����
void TestSocket::closeSocket()
{
	if (_sioClient) {
		//	�ر�socket����,��ɺ����ô�����onClose()
		_sioClient->disconnect();
	}
}
//	����socket����ʱ������������
void TestSocket::onConnect(cocos2d::network::SIOClient* client)
{
	log("TestSocket::onConnect called");
}
//	�����յ�����ʱ��������������
void TestSocket::onMessage(cocos2d::network::SIOClient* client, const std::string& data)
{
	log("TestSocket::onMessage received: %s", data.c_str());
}
//	��socket�ر�ʱ��������������
void TestSocket::onClose(cocos2d::network::SIOClient* client)
{
	CCLOG("TestSocket::onClose called");
}
//	�����Ӵ���ʱ������������
void TestSocket::onError(cocos2d::network::SIOClient* client, const std::string& data)
{
	log("Error received: %s", data.c_str());
}
void TestSocket::testevent(SIOClient *client, const std::string& data)
{
	log("TestSocket::testevent called with data: %s", data.c_str());
}
void TestSocket::echotest(SIOClient *client, const std::string& data)
{
	log("TestSocket::echotest called with data: %s", data.c_str());
}


/////////////////////////////////////////////////////// WebSocket /////////////////////////////////////////////////
void TestWebSocket::initTestWebSocket()
{
	auto winSize = Director::getInstance()->getVisibleSize();
	//	�������
	_sendTextTimes = 0;
	_sendBinaryTimes = 0;
	auto menuRequest = Menu::create();
	menuRequest->setPosition(Vec2::ZERO);
	addChild(menuRequest);

	// ��WebScoket����
	auto openItem = MenuItemFont::create("1 Open WebSocket", CC_CALLBACK_1(TestWebSocket::onMenuClicked, this));
	openItem->setPosition(Vec2(winSize.width / 2, winSize.height / 2 + 100));
	openItem->setTag(ONE);
	menuRequest->addChild(openItem);

	//	������������ַ�����Ϣ
	auto itemSendText = MenuItemFont::create("2 Send Text", CC_CALLBACK_1(TestWebSocket::onMenuClicked, this));
	itemSendText->setPosition(Vec2(winSize.width / 2, winSize.height / 2));
	itemSendText->setTag(TWO);
	menuRequest->addChild(itemSendText);

	// Send Binary
	auto itemSendBinary = MenuItemFont::create("3 Send Binary", CC_CALLBACK_1(TestWebSocket::onMenuClicked, this));
	itemSendBinary->setPosition(Vec2(winSize.width / 2, winSize.height / 2 - 100));
	itemSendBinary->setTag(THREE);
	menuRequest->addChild(itemSendBinary);

	//	�ر�WebSocket
	auto colseItem = MenuItemFont::create("4 Close WebSocket", CC_CALLBACK_1(TestWebSocket::onMenuClicked, this));
	colseItem->setPosition(Vec2(winSize.width / 2, winSize.height / 2 - 200));
	colseItem->setTag(FOUR);
	menuRequest->addChild(colseItem);

	//	����
	auto backItem = MenuItemFont::create("Back", CC_CALLBACK_1(TestWebSocket::onMenuClicked, this));
	backItem->setPosition(Vec2(winSize.width*0.9, 50));
	menuRequest->addChild(backItem);
}

Scene* TestWebSocket::scene()
{
	auto scene = Scene::create();
	auto layer = new TestWebSocket();
	scene->addChild(layer);

	layer->initTestWebSocket();
	return scene;
}

void  TestWebSocket::onMenuClicked(Ref* pObj)
{
	int tag = ((MenuItem*)pObj)->getTag();
	if (tag == ONE)
	{
		//	����3��WebSocket����
		_wsiSendText = new WebSocket();
		_wsiSendBinary = new WebSocket();
		_wsiError = new WebSocket();
		//	*this ����webSocket���ݵĴ�����(����)��"ws://echo.websocket.org"��������ַ
		_wsiSendText->init(*this, "ws://echo.websocket.org");
		_wsiSendBinary->init(*this, "ws://echo.websocket.org");
		//	����Error,�����������ַ��
		_wsiError->init(*this, "ws://invalid.url.com");
	}
	else if (tag == TWO)
	{
		if (!_wsiSendText)
		{
			return;
		}

		//	����getReadyState()��������ȡ����״̬�����жԱ�;�����OPEN,��ö��ֵ�������ֱ��� CONNECTING,CLOSING,CLOSED��
		if (_wsiSendText->getReadyState() == network::WebSocket::State::OPEN)
		{
			CCLOG("�Ѿ��򿪳ɹ����������ַ�����Ϣ");
			//	������������ַ�����Ϣ
			_wsiSendText->send("Hello WebSocket, I'm a text message.");
		}
		else
		{
			std::string warningStr = "send text websocket instance wasn't ready...";
			log("%s", warningStr.c_str());
		}
	}
	else if (tag == THREE)
	{
		if (!_wsiSendBinary)
		{
			return;
		}
		//	����getReadyState()��������ȡ����״̬�����жԱ�;�����OPEN,��ö��ֵ�������ֱ��� CONNECTING,CLOSING,CLOSED��
		if (_wsiSendBinary->getReadyState() == network::WebSocket::State::OPEN)
		{
			CCLOG("�Ѿ��򿪳ɹ����������ַ�����Ϣ");
			//	����������Ͷ�������Ϣ
			char buf[] = "Hello WebSocket,\0 I'm\0 a\0 binary\0 message\0.";
			_wsiSendBinary->send((unsigned char*)buf, sizeof(buf));
		}
		else
		{
			std::string warningStr = "send binary websocket instance wasn't ready...";
			log("%s", warningStr.c_str());
		}
	}
	else if (tag == FOUR)
	{
		if (_wsiSendText)
		{
			//	���ùر�
			_wsiSendText->close();
		}
		if (_wsiSendBinary)
		{
			_wsiSendBinary->close();
		}
		if (_wsiError)
		{
			_wsiError->close();
		}
	}
	else
	{
		Director::getInstance()->replaceScene(TestNetWork::createScene());
	}
}

//	����WebSocket����ʱ������������
void TestWebSocket::onOpen(WebSocket* ws)
{
	//	������Ӵ���Ϣ
	log("Websocket (%p) opened", ws);
	if (ws == _wsiSendText)
	{
		MessageBox("Send Text WS was opened.\nws://echo.websocket.org", "��ʾ");
	}
	else if (ws == _wsiSendBinary)
	{
		MessageBox("Send Binary WS was opened.\nws://echo.websocket.org.", "��ʾ");
	}
	else if (ws == _wsiError)
	{
		MessageBox("error test will never go here.\nws://invalid.url.com", "��ʾ");
		CCASSERT(0, "error test will never go here.");
	}
}

//	������ص�����
void TestWebSocket::onMessage(WebSocket* ws, const WebSocket::Data& data)
{
	//	�ж��ǲ��������Ƿ���2���ƣ����ò�ͬ�����������
	if (!data.isBinary)
	{//	�Ƕ��������
		_sendTextTimes++;
		char times[100] = { 0 };
		sprintf(times, "%d", _sendTextTimes);
		std::string textStr = std::string("response text msg: ") + data.bytes + ", " + times;
		CCLOG("%s", textStr.c_str());
		std::string textStr1 = std::string("�ı����:\nresponse text msg: ") + data.bytes + ", " + times;
		MessageBox(textStr1.c_str(), "��ʾ");
	}
	else
	{//	���������
		_sendBinaryTimes++;
		char times[100] = { 0 };
		sprintf(times, "%d", _sendBinaryTimes);
		std::string binaryStr = "response bin msg: ";
		for (int i = 0; i < data.len; ++i)
		{
			if (data.bytes[i] != '\0')
			{
				binaryStr += data.bytes[i];
			}
			else
			{
				binaryStr += "\'\\0\'";
			}
		}

		binaryStr += std::string(", ") + times;
		CCLOG("%s", binaryStr.c_str());

		std::string textStr = std::string("�������������:\n") + binaryStr.c_str();
		MessageBox(textStr.c_str(), "��ʾ");
	}
}
//	�ر�WebSocket����
void TestWebSocket::onClose(WebSocket* ws)
{
	log("websocket instance (%p) closed.", ws);
	if (ws == _wsiSendText)
	{
		_wsiSendText = nullptr;
	}
	else if (ws == _wsiSendBinary)
	{
		_wsiSendBinary = nullptr;
	}
	else if (ws == _wsiError)
	{
		_wsiError = nullptr;
	}
	//	ɾ������
	CC_SAFE_DELETE(ws);
}

//	������Ϣ
void TestWebSocket::onError(WebSocket* ws, const WebSocket::ErrorCode& error)
{
	char buf[100] = { 0 };
	sprintf(buf, "an error was fired, code:\n %d    ,%p", error, ws);
	MessageBox(buf, "��ʾ");
	CCLOG("Error was fired, error code: %d", error);
}