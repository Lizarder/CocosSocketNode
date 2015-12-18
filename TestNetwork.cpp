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

//	创建按钮
void TestNetWork::createButton()
{
	//	GET 请求按钮
	auto getItem = MenuItemFont::create("TestNetWork --HTTP GET", CC_CALLBACK_1(TestNetWork::menuClickCallback, this));
	getItem->setPosition(Vec2(size.width / 2, size.height / 2 + 140));
	getItem->setTag(ONE);

	//	POST 请求按钮
	auto postItem = MenuItemFont::create("TestNetWork --HTTP POST", CC_CALLBACK_1(TestNetWork::menuClickCallback, this));
	postItem->setPosition(Vec2(size.width / 2, size.height / 2 + 70));
	postItem->setTag(TWO);

	//	Socket 请求按钮
	auto socketItem = MenuItemFont::create("TestNetWork -- Socket", CC_CALLBACK_1(TestNetWork::menuClickCallback, this));
	socketItem->setPosition(Vec2(size.width / 2, size.height / 2));
	socketItem->setTag(THREE);

	//	Socket 请求按钮
	auto webSocketItem = MenuItemFont::create("TestNetWork -- WebSocket", CC_CALLBACK_1(TestNetWork::menuClickCallback, this));
	webSocketItem->setPosition(Vec2(size.width / 2, size.height / 2 - 70));
	webSocketItem->setTag(FOUR);

	//	返回
	auto backItem = MenuItemFont::create("Back", CC_CALLBACK_1(TestNetWork::menuClickCallback, this));
	backItem->setPosition(Vec2(size.width*0.9, 50));

	//	创建菜单
	auto menu = Menu::create(getItem, postItem, socketItem, webSocketItem, backItem, NULL);
	menu->setPosition(Vec2::ZERO);
	this->addChild(menu, 1);
}

//	按钮响应函数
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

//	GET通信
void TestNetWork::createGetHttp()
{
	//	生成HttpRequest对象
	auto request = new HttpRequest();
	//	设置请求
	request->setUrl("http://www.baidu.com");
	//	设置请求方式  GET类型
	request->setRequestType(HttpRequest::Type::GET);
	//	设置请求完成后的回调函数
	request->setResponseCallback(CC_CALLBACK_2(TestNetWork::getHttp_handshakeResponse, this));
	//	设置请求tag
	request->setTag("getHttp_handshake");
	//	生成HttpClient对象，并且发送请求
	HttpClient::getInstance()->send(request);
	//	释放HttpRequest对象
	request->release();
}
void TestNetWork::getHttp_handshakeResponse(HttpClient *sender, HttpResponse *response)
{
	//	通过response 获取请求tag，能获取到说明请求发出，并且输出请求tag值
	if (0 != strlen(response->getHttpRequest()->getTag()))
	{
		CCLOG("request tag: %s", response->getHttpRequest()->getTag());
	}

	//	判断是否请求成功 方法一: 获取请求状态码
	long codeActive = response->getResponseCode();
	if (codeActive == 200)
	{
		MessageBox("请求发送成功", "提示");
		CCLOG("%ld", codeActive);
	}
	else
	{
		//	response->getErrorBuffer() 返回错误代码
		auto erroeStr = String::createWithFormat("请求发送失败! \n 错误代码: %s", response->getErrorBuffer());
		MessageBox(erroeStr->getCString(), "提示");
		CCLOG("%ld", codeActive);
	}

	//	判断是否请求成功 方法二:使用HttpResponse对象提供的函数isSucceed() 进行判断(返回值bool类型)
	if (!response->isSucceed())
	{
		//	response->getErrorBuffer() 返回错误代码
		auto erroeStr = String::createWithFormat("请求发送失败! \n 错误代码: %s", response->getErrorBuffer());
		MessageBox(erroeStr->getCString(), "提示");
		return;
	}

	//	使用response->getResponseData(); 获取请求返回的数据
	std::vector<char> *buffer = response->getResponseData();
	std::stringstream s;

	//	输出数据
	for (unsigned int i = 0; i < buffer->size(); i++)
	{
		s << (*buffer)[i];
	}
	CCLOG("data: %s", s.str().c_str());
}

//	POST通信
void TestNetWork::createPostHttp()
{
	//	从 testCpp中可以找到源码
	//	生成HttpRequest对象
	HttpRequest* request = new HttpRequest();
	//	设置请求
	request->setUrl("http://localhost:8080");
	//	设置请求方式  GET类型
	request->setRequestType(HttpRequest::Type::POST);
	//	选择性的设置Http请求的Content-Type
	std::vector<std::string> headers;
	headers.push_back("Content-Type: application/json; charset=utf-8");
	request->setHeaders(headers);
	//	设置请求完成后的回调函数
	request->setResponseCallback(CC_CALLBACK_2(TestNetWork::PostHttp_handshakeResponse, this));

	// 使用setRequestData()函数设置提交数据
	const char* postData = "visitor=cocos2d&TestSuite=Extensions Test/NetworkTest";
	request->setRequestData(postData, strlen(postData));
	//	设置请求tag
	request->setTag("postHttp_handshake");
	//	生成HttpClient对象，并且发送请求
	HttpClient::getInstance()->send(request);
	//	释放HttpRequest对象
	request->release();
}
void TestNetWork::PostHttp_handshakeResponse(HttpClient *sender, HttpResponse *response)
{
	//	通过response 获取请求tag，能获取到说明请求发出，并且输出请求tag值
	if (0 != strlen(response->getHttpRequest()->getTag()))
	{
		CCLOG("request tag: %s", response->getHttpRequest()->getTag());
	}

	//	请求码
	long statusCode = response->getResponseCode();
	CCLOG("response code: %ld", statusCode);

	//	判断请求是否成功
	if (!response->isSucceed())
	{
		//	response->getErrorBuffer() 返回错误代码
		auto erroeStr = String::createWithFormat("请求发送失败! \n 错误代码: %s", response->getErrorBuffer());
		MessageBox(erroeStr->getCString(), "提示");
		return;
	}

	// 输出返回的数据
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

	//	返回
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
//	打开Socket连接
void TestSocket::createSocket()
{
	//	 打开Socket连接
	_sioClient = SocketIO::connect("ws://channon.us:3000", *this);
	//	设置标记请求
	_sioClient->setTag("Test Client");

	//	注册socket.io事件回调函数
	_sioClient->on("testevent", CC_CALLBACK_2(TestSocket::testevent, this));
	_sioClient->on("echotest", CC_CALLBACK_2(TestSocket::echotest, this));
}
//	向服务器发送消息
void TestSocket::sendMessage()
{
	if (_sioClient) {
		//	发送消息到socket.Io服务器
		_sioClient->send("Hello Socket.IO!");
	}
}
//	委托处理socket.Io服事件，利用emit触发echotest事件
void TestSocket::sendEvent()
{
	if (_sioClient) {
		//	委托处理socket.Io服事件，利用emit触发echotest事件
		_sioClient->emit("echotest", "[{\"name\":\"myname\",\"type\":\"mytype\"}]");
	}
}
//	关闭socket连接
void TestSocket::closeSocket()
{
	if (_sioClient) {
		//	关闭socket连接,完成后会调用代理函数onClose()
		_sioClient->disconnect();
	}
}
//	当打开socket连接时会调用这个函数
void TestSocket::onConnect(cocos2d::network::SIOClient* client)
{
	log("TestSocket::onConnect called");
}
//	当接收到数据时会调用这个函数，
void TestSocket::onMessage(cocos2d::network::SIOClient* client, const std::string& data)
{
	log("TestSocket::onMessage received: %s", data.c_str());
}
//	当socket关闭时，会调用这个函数
void TestSocket::onClose(cocos2d::network::SIOClient* client)
{
	CCLOG("TestSocket::onClose called");
}
//	当连接错误时会调用这个函数
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
	//	用于输出
	_sendTextTimes = 0;
	_sendBinaryTimes = 0;
	auto menuRequest = Menu::create();
	menuRequest->setPosition(Vec2::ZERO);
	addChild(menuRequest);

	// 打开WebScoket链接
	auto openItem = MenuItemFont::create("1 Open WebSocket", CC_CALLBACK_1(TestWebSocket::onMenuClicked, this));
	openItem->setPosition(Vec2(winSize.width / 2, winSize.height / 2 + 100));
	openItem->setTag(ONE);
	menuRequest->addChild(openItem);

	//	向服务器发送字符串消息
	auto itemSendText = MenuItemFont::create("2 Send Text", CC_CALLBACK_1(TestWebSocket::onMenuClicked, this));
	itemSendText->setPosition(Vec2(winSize.width / 2, winSize.height / 2));
	itemSendText->setTag(TWO);
	menuRequest->addChild(itemSendText);

	// Send Binary
	auto itemSendBinary = MenuItemFont::create("3 Send Binary", CC_CALLBACK_1(TestWebSocket::onMenuClicked, this));
	itemSendBinary->setPosition(Vec2(winSize.width / 2, winSize.height / 2 - 100));
	itemSendBinary->setTag(THREE);
	menuRequest->addChild(itemSendBinary);

	//	关闭WebSocket
	auto colseItem = MenuItemFont::create("4 Close WebSocket", CC_CALLBACK_1(TestWebSocket::onMenuClicked, this));
	colseItem->setPosition(Vec2(winSize.width / 2, winSize.height / 2 - 200));
	colseItem->setTag(FOUR);
	menuRequest->addChild(colseItem);

	//	返回
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
		//	创建3个WebSocket对象
		_wsiSendText = new WebSocket();
		_wsiSendBinary = new WebSocket();
		_wsiError = new WebSocket();
		//	*this 接收webSocket数据的代理类(本类)，"ws://echo.websocket.org"服务器地址
		_wsiSendText->init(*this, "ws://echo.websocket.org");
		_wsiSendBinary->init(*this, "ws://echo.websocket.org");
		//	测试Error,这个服务器地址错
		_wsiError->init(*this, "ws://invalid.url.com");
	}
	else if (tag == TWO)
	{
		if (!_wsiSendText)
		{
			return;
		}

		//	利用getReadyState()函数来获取链接状态，进行对比;这里额OPEN,是枚举值；其他分别是 CONNECTING,CLOSING,CLOSED等
		if (_wsiSendText->getReadyState() == network::WebSocket::State::OPEN)
		{
			CCLOG("已经打开成功，正发送字符串消息");
			//	向服务器发送字符串消息
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
		//	利用getReadyState()函数来获取链接状态，进行对比;这里额OPEN,是枚举值；其他分别是 CONNECTING,CLOSING,CLOSED等
		if (_wsiSendBinary->getReadyState() == network::WebSocket::State::OPEN)
		{
			CCLOG("已经打开成功，正发送字符串消息");
			//	向服务器发送二进制消息
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
			//	调用关闭
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

//	当打开WebSocket连接时会调用这个函数
void TestWebSocket::onOpen(WebSocket* ws)
{
	//	输出链接打开消息
	log("Websocket (%p) opened", ws);
	if (ws == _wsiSendText)
	{
		MessageBox("Send Text WS was opened.\nws://echo.websocket.org", "提示");
	}
	else if (ws == _wsiSendBinary)
	{
		MessageBox("Send Binary WS was opened.\nws://echo.websocket.org.", "提示");
	}
	else if (ws == _wsiError)
	{
		MessageBox("error test will never go here.\nws://invalid.url.com", "提示");
		CCASSERT(0, "error test will never go here.");
	}
}

//	输出返回的数据
void TestWebSocket::onMessage(WebSocket* ws, const WebSocket::Data& data)
{
	//	判断是不是数据是否是2进制，利用不同方法输出数据
	if (!data.isBinary)
	{//	非二进制输出
		_sendTextTimes++;
		char times[100] = { 0 };
		sprintf(times, "%d", _sendTextTimes);
		std::string textStr = std::string("response text msg: ") + data.bytes + ", " + times;
		CCLOG("%s", textStr.c_str());
		std::string textStr1 = std::string("文本输出:\nresponse text msg: ") + data.bytes + ", " + times;
		MessageBox(textStr1.c_str(), "提示");
	}
	else
	{//	二进制输出
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

		std::string textStr = std::string("二进制类型输出:\n") + binaryStr.c_str();
		MessageBox(textStr.c_str(), "提示");
	}
}
//	关闭WebSocket连接
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
	//	删除操作
	CC_SAFE_DELETE(ws);
}

//	错误信息
void TestWebSocket::onError(WebSocket* ws, const WebSocket::ErrorCode& error)
{
	char buf[100] = { 0 };
	sprintf(buf, "an error was fired, code:\n %d    ,%p", error, ws);
	MessageBox(buf, "提示");
	CCLOG("Error was fired, error code: %d", error);
}