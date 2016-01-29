#ifndef _POPUPVIEW_H
#define _POPUPVIEW_H

#include "cocos2d.h"
#include "cocos-ext.h"
#include "ui/CocosGUI.h"
#include "cocostudio/CocoStudio.h"
#include "cocos-ext.h"

USING_NS_CC;
USING_NS_CC_EXT;

class PopUpVew : public cocos2d::LayerColor
{
public:
	PopUpVew();
	~PopUpVew();

	virtual bool init();
	CREATE_FUNC(PopUpVew);

	static PopUpVew* create(const char* bg);
	virtual void onEnter();
	virtual void onExit();

	void scrollViewCallBack(Ref* ref, ui::ScrollView::EventType type);
	void buttonCallBack1(Ref* ref, ui::Widget::TouchEventType type);
	void buttonClicked1(Ref* ref);
private:
	Rect viewRect;
	
};
#endif