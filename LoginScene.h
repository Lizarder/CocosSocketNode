#ifndef _LOGIN_SCENE_H_
#define _LOGIN_SCENE_H_

#include "cocos2d.h"


class LoginScene : public cocos2d::Layer{
public:
	static cocos2d::Scene* createScene();
	virtual bool init();
	CREATE_FUNC(LoginScene);
};

#endif