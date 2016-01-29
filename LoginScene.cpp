#include "cocos2d.h"
#include "LoginScene.h"
#include "CustomDialog.h"
#include "TestNetwork.h"
#include "SocketNode.h"
#include "PopUpView.h"

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
	Size visibleSize = Director::getInstance()->getVisibleSize();
	float screenW = visibleSize.width;
	float screenH = visibleSize.height;

	Menu* menu = Menu::create();
	menu->setPosition(Vec2(0, 0));
	this->addChild(menu);

	MenuItemLabel* loginButton = MenuItemLabel::create(Sprite::create("CloseNormal.png"), this, CC_MENU_SELECTOR(LoginScene::loginCallBack));
	MenuItemLabel* popView = MenuItemLabel::create(Sprite::create("CloseSelected.png"), this, CC_MENU_SELECTOR(LoginScene::popViewCallBack));

	loginButton->setPosition(Vec2(screenW/5, screenH/3));
	popView->setPosition(Vec2(screenW / 5+60, screenH / 3));
	menu->addChild(loginButton);
	menu->addChild(popView);
	
	setTouchEnabled(true);
	return ret;
}

void LoginScene::loginCallBack(cocos2d::Ref* pSender)
{
	/*CustomDialog* customDialog = CustomDialog::create("DialogBG.png");
	customDialog->setContentSize(Size(200, 150));
	customDialog->setTitle("DialogCustom");
	customDialog->setContent("oh my god !");

	this->addChild(customDialog);*/
	//Director::getInstance()->replaceScene(TestNetWork::createScene());
	socketNode = SocketNode::create("192.168.3.157", 12345, this, static_cast<SocketNode::SEL_CallFuncUDU>(&LoginScene::onReceiveData));
	this->addChild(socketNode, -10, "SocketNode");
}

void LoginScene::popViewCallBack(cocos2d::Ref* pSender)
{
	auto popUpView = PopUpVew::create("dialog_scale.png");
	popUpView->setPosition(Vec2::ZERO);
	this->addChild(popUpView);
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