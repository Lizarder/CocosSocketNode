#ifndef _LOGIN_SCENE_H_
#define _LOGIN_SCENE_H_

#include "cocos2d.h"
#include "SocketNode.h"

class LoginScene : public cocos2d::Layer{
public:
	static cocos2d::Scene* createScene();
	virtual bool init();
	CREATE_FUNC(LoginScene);
	void loginCallBack(cocos2d::Ref* pSender);
	void popViewCallBack(cocos2d::Ref* pSender);
	void onReceiveData(unsigned int type, void* pData, unsigned int len);
private:
	SocketNode *socketNode;
};

#endif