#ifndef _DIALOG_H_
#define _DIALOG_H

#include "cocos2d.h"
#include "cocos-ext.h"
USING_NS_CC;
USING_NS_CC_EXT;

class CustomDialog : public cocos2d::Layer
{
public:
	CustomDialog();
	~CustomDialog();

	virtual bool init();
	CREATE_FUNC(CustomDialog);

	static CustomDialog* create(const char* backGroundImg);
	void setTitle(const char* title, int fontsize = 20);
	void setContent(const char* content, int fontsize = 20);

	virtual void onEnter();
	virtual void onExit();

private:
	void buttonOnClicked(Ref* pSender);

	CC_SYNTHESIZE_RETAIN(Menu*, _menu,MenuButton);
	CC_SYNTHESIZE_RETAIN(Sprite*, _background, BackGroundSprite);
	CC_SYNTHESIZE_RETAIN(Scale9Sprite*, _scale9backGround, Scale9BackGround);
	CC_SYNTHESIZE_RETAIN(LabelTTF*, _textTitle, TitleLabel);
	CC_SYNTHESIZE_RETAIN(LabelTTF*, _textContent, ContentLabel,int padding);
	int _contentPaddingLR;
	int _contentPaddingTB;
};

#endif