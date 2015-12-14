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

	CustomDialog* customDialog = CustomDialog::create("DialogBG.png");
	customDialog->setTitle("DialogCustom");
	customDialog->setContent("oh my god !");

	this->addChild(customDialog);

	return ret;
}