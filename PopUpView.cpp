#include "cocos2d.h"
#include "PopUpView.h"
#include "ui/CocosGUI.h"
#include "cocostudio/CocoStudio.h"
#include "cocos-ext.h"

USING_NS_CC;
using namespace ui;

PopUpVew::PopUpVew()
{
	viewRect = Rect(0, 0, 0, 0);
}

PopUpVew::~PopUpVew()
{
}

bool PopUpVew::init()
{
	if (!LayerColor::initWithColor(Color4B(0,0,0,150)))
	{
		return false;
	}
	setTouchEnabled(true);
	return true;
}

PopUpVew* PopUpVew::create(const char* bg)
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
	auto visibleSize = Director::getInstance()->getVisibleSize();


	Layer::onEnter();

	auto layout = Layout::create();
	layout->setContentSize(Size(visibleSize.width/2, visibleSize.height*2/3));
	layout->setBackGroundImageScale9Enabled(true);
	layout->setBackGroundImage("dialog_scale.png");
	layout->setPosition(Vec2(visibleSize.width/3,0));
	this->addChild(layout);

	ui::ScrollView* scollView = ui::ScrollView::create();
	scollView->setContentSize(Size(visibleSize.width / 3, visibleSize.height * 2 / 3));
	scollView->setAnchorPoint(Vec2(0.5, 0));
	scollView->setPosition(Vec2(layout->getContentSize().width/2,0));

	auto innerSize = Size(scollView->getContentSize().width, scollView->getContentSize().height+200);
	scollView->setInnerContainerSize(innerSize);

	auto button1 = ui::Button::create("PlayButton.png");
	button1->setPosition(Vec2(innerSize.width / 2, innerSize.height-120));
	button1->addClickEventListener(CC_CALLBACK_1(PopUpVew::buttonClicked1, this));

	log(button1->getContentSize().height);
	scollView->addChild(button1);

	auto button2 = ui::Button::create("PlayButton.png");
	button2->setPosition(Vec2(innerSize.width / 2, innerSize.height - 260));
	scollView->addChild(button2);

	auto button3 = ui::Button::create("PlayButton.png");
	button3->setPosition(Vec2(innerSize.width / 2, innerSize.height - 400));
	scollView->addChild(button3);

	auto button4 = ui::Button::create("PlayButton.png");
	button4->setPosition(Vec2(innerSize.width / 2, innerSize.height - 540));
	scollView->addChild(button4);

	Menu* menu = Menu::create();
	menu->setPosition(Vec2(0, 0));
	scollView->addChild(menu);

	
	MenuItemLabel* menuItem = MenuItemLabel::create(Sprite::create("CloseNormal.png"), this, CC_MENU_SELECTOR(PopUpVew::buttonClicked1));
	menuItem->setPosition(Vec2(innerSize.width / 2, innerSize.height - 60));
	menu->addChild(menuItem);
	
	scollView->addEventListener(CC_CALLBACK_2(PopUpVew::scrollViewCallBack,this));
	layout->addChild(scollView);

	log("PopUpVew::onEnter()");

	
}

void PopUpVew::buttonCallBack1(Ref* ref, Widget::TouchEventType type)
{
	
	log("button1 touch began");
	
}
void PopUpVew::buttonClicked1(Ref* ref)
{

	log("button1 touch began");

}

void PopUpVew::scrollViewCallBack(Ref* ref, ui::ScrollView::EventType type)
{
	switch(type)
	{
	case  ui::ScrollView::EventType::SCROLLING:
		log("scrollview scrolling");
		break;
	case  ui::ScrollView::EventType::SCROLL_TO_BOTTOM :
		log("scrollview scroll to buttom");
		break;
	case ui::ScrollView::EventType::SCROLL_TO_TOP :
		log("scrollniew scroll to top");
		break;
	}
}