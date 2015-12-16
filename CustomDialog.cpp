#include "cocos2d.h"
#include "CustomDialog.h"

USING_NS_CC;

CustomDialog::CustomDialog() :
_menu(nullptr),
_background(nullptr),
_scale9backGround(nullptr),
_textTitle(nullptr),
_textContent(nullptr),
_contentPaddingLR(0),
_contentPaddingTB(0)
{

}

CustomDialog::~CustomDialog()
{
	CC_SAFE_RELEASE(_menu);
	CC_SAFE_RELEASE(_background);
	CC_SAFE_RELEASE(_scale9backGround);
	CC_SAFE_RELEASE(_textTitle);
	CC_SAFE_RELEASE(_textContent);
}

bool CustomDialog::init()
{
	if (!Layer::init())
	{
		return false;
	}
	Menu* impl = Menu::create();
	impl->setPosition(Vec2::ZERO);
	setMenuButton(impl);

	setTouchEnabled(true);

	return true;
}

CustomDialog* CustomDialog::create(const char* backGroundImg)
{
	CustomDialog* _instance = CustomDialog::create();
	_instance->setBackGroundSprite(Sprite::create(backGroundImg));
	_instance->setScale9BackGround(Scale9Sprite::create(backGroundImg));
	return _instance;
}

void CustomDialog::setTitle(const char* title, int fontsize /* = 30 */)
{
	LabelTTF* impl = LabelTTF::create(title, "", fontsize);
	this->setTitleLabel(impl);
}

void CustomDialog::setContent(const char* content, int fontsize /* = 30 */)
{
	LabelTTF* impl = LabelTTF::create(content, "", fontsize);
	this->setContentLabel(impl);

}

void CustomDialog::onEnter()
{
	Layer::onEnter();

	Size winSize = Director::getInstance()->getWinSize();
	Size visibleSize = Director::getInstance()->getVisibleSize();

	Vec2 pCenter = Vec2(winSize.width / 2, winSize.height / 2);
	Size dgContentSize;
	if (getContentSize().equals(Size(0, 0)))
	{
		getBackGroundSprite()->setPosition(pCenter);
		this->addChild(getBackGroundSprite(), 0, "background");
		dgContentSize = getBackGroundSprite()->getTexture()->getContentSize();

	}
	else
	{
		Scale9Sprite *background = getScale9BackGround();
		background->setContentSize(getContentSize());
		background->setPosition(pCenter);
		this->addChild(background, 0, "background");
		dgContentSize = getContentSize();
	}

	this->addChild(getMenuButton());

	/*��Ӱ�ťδʵ��*/
	Vec2 bgCenter = Vec2(dgContentSize.width / 2, dgContentSize.height / 2);
	if (getTitleLabel())//����
	{
		getTitleLabel()->setPosition(pCenter + Vec2(0, dgContentSize.height/2 - 10));
		this->addChild(getTitleLabel());
	}

	if (getContentLabel())//����
	{
		getContentLabel()->setPosition(pCenter + Vec2(0, dgContentSize.height/2 - 60));
		this->addChild(getContentLabel());
	}

	Action* popupAction = Sequence::create(ScaleTo::create(0.0, 0.0),
		ScaleTo::create(3, 1.05), ScaleTo::create(3, 0.85), ScaleTo::create(3, 1.0), NULL);
	
	this->runAction(popupAction);

}

void CustomDialog::onExit()
{
	log("CustomDialog::onExit()");
	Layer::onExit();
}