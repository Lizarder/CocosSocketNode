#ifndef _POPUPVIEW_H
#define _POPUPVIEW_H

#include "cocos2d.h"
#include "cocos-ext.h"

USING_NS_CC;
USING_NS_CC_EXT;

class PopUpVew : public cocos2d::Layer
{
public:
	PopUpVew();
	~PopUpVew();

	virtual bool init();
	CREATE_FUNC(PopUpVew);

	static PopUpVew* create();
	virtual void onEnter();
	virtual void onExit();

private:
	
};
#endif