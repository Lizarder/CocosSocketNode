#include "cocos2d.h"
#include "LoginScene.h"
#include "CustomDialog.h"

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

	setTouchEnabled(true);
	return ret;
}

void LoginScene::menuCallBack(cocos2d::Ref* pSender)
{
	CustomDialog* customDialog = CustomDialog::create("dialog_scale.png");
	customDialog->setContentSize(Size(200, 150));
	customDialog->setTitle("DialogCustom");
	customDialog->setContent("oh my god !");

	this->addChild(customDialog);
}