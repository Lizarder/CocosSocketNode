#include "cocos2d.h"
#include "PopUpView.h"
#include "ui/CocosGUI.h"
#include "cocostudio/CocoStudio.h"
#include "cocos-ext.h"

USING_NS_CC;
using namespace ui;

PopUpVew::PopUpVew()
{

}

PopUpVew::~PopUpVew()
{
}

bool PopUpVew::init()
{
	if (!Layer::init())
	{
		return false;
	}
	return true;
}

PopUpVew* PopUpVew::create()
{
	PopUpVew* _instance = PopUpVew::create();
	return _instance;
}

void PopUpVew::onExit()
{
	log("PopUpVew::onExit()");
	Layer::onExit();
}

void PopUpVew::onEnter()
{
	log("PopUpVew::onEnter()");
	Layer::onEnter();
}