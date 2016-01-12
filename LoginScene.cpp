#include "cocos2d.h"
#include "LoginScene.h"
#include "CustomDialog.h"
#include "TestNetwork.h"
#include "SocketNode.h"

USING_NS_CC;

Scene* LoginScene::createScene()
{
	auto scene = Scene::create();

	auto layer = LoginScene::create();

	scene->addChild(layer);

	return scene;

}

bool LoginScene::init()
{
	bool ret = true;
	if (!Layer::init())
	{
		return false;
	}

	Menu* menu = Menu::create();
	menu->setPosition(Vec2(0, 0));
	this->addChild(menu);

	MenuItemLabel* popupDialog = MenuItemLabel::create(Sprite::create("CloseNormal.png"),this,CC_MENU_SELECTOR(LoginScene::menuCallBack));
	
	popupDialog->setPosition(Vec2(50, 50));
	menu->addChild(popupDialog);

	socketNode = SocketNode::create("192.168.3.157", 60000, this, static_cast<SocketNode::SEL_CallFuncUDU>(&LoginScene::onReceiveData));
	this->addChild(socketNode,-10,"SocketNode");
	

	setTouchEnabled(true);
	return ret;
}

void LoginScene::menuCallBack(cocos2d::Ref* pSender)
{
	/*CustomDialog* customDialog = CustomDialog::create("DialogBG.png");
	customDialog->setContentSize(Size(200, 150));
	customDialog->setTitle("DialogCustom");
	customDialog->setContent("oh my god !");

	this->addChild(customDialog);*/
	//Director::getInstance()->replaceScene(TestNetWork::createScene());
	char *content = "client 123456789";
	socketNode->sendData(content, strlen(content)+1);
}

void LoginScene::onReceiveData(unsigned int type, void* pData, unsigned int len)
{
	switch (type)
	{
	case SocketNode::NOTIFY_CONNECTED:
		cocos2d::log("onReceiveData : notify_connect"); 
		cocos2d::log((char*)pData);
		break;
	case SocketNode::NOTIFY_CONNECT_FAILED:
		cocos2d::log("onReceiveData : notify_connect_failed");
		cocos2d::log((char*)pData);
		break;
	case SocketNode::NOTIFY_DISCONNECTED:
	{
		cocos2d::log("onReceiveData : notify_disconnect");
		cocos2d::log((char*)pData);
	}
	break;
	case SocketNode::NOTIFY_PACKET:
		cocos2d::log("onReceiveData : notify_packet");
		cocos2d::log((char*)pData);
		break;
	}
}