
#ifndef __CC_HTTP_DOWNLOAD_NODE_H_
#define __CC_HTTP_DOWNLOAD_NODE_H_

#define CC_LUA_ENGINE_ENABLED 1

#include "cocos2d.h"

#ifdef _WINDOWS_
#include <Windows.h>
#else
#include <pthread.h>
#endif

#include <stdio.h>
#include <vector>
#include <map>
#include <string>

namespace lk {


class HttpDownloadNode : public cocos2d::Node {
private:
	class Impl;
	friend class Impl;
	Impl* impl;

	HttpDownloadNode();
	~HttpDownloadNode();
public:

	static const unsigned int NOTIFY_CONNECTED = 1;
	static const unsigned int NOTIFY_SIZE = 2;
	static const unsigned int NOTIFY_FILENAME = 3;
	static const unsigned int NOTIFY_PROGRESS = 4;
	static const unsigned int NOTIFY_FAILED = 5;
	static const unsigned int NOTIFY_SUCCEEDED = 6;

	typedef void (cocos2d::Ref::*SEL_CallFuncUDU)(unsigned int, void*, unsigned int);

	// ����ĳ����ַ���ļ���������ɺ��ԴӴ�parent node���Ƴ�����
	// onNotify���Խ��ս���֪ͨ�����֪ͨ: onNotify(eventType, data); ��Ϣ�б�Ϊ��
	//     "connected", nil; "size", sizeInBytes; "filename", filename; "progress", sizeInBytes; 
	//     "failed", reason, msg; "succeeded", nil/content;    // contentֻ��savePathNameΪ��ʱ���͡�
	// savePathName��ָ������·�����벻Ҫ���������ָ��Ϊͬһ������·������Ȼ�ļ������п����������������ݵĻ�ϣ����³���
	// ��ָ��savePathNameʱ��"succeeded"��Ϣ�ĵڶ��������������ص�HTTP���ݡ�
	// ���ָ����savePathName, bytes�����Զ�֧�ֶϵ��������ܡ��ϵ�������Ϣ�ļ���������savePathName+".info"�У�������������ɺ�ɾ����
	// ���ָ����bytes��/��md5Val������������ɺ���֤�ļ���С��md5ֵ��
	// ���ָ����extractToFolder�������԰�zip�ļ���ʽ�������ص��ļ���ѹ��extractToFolder�ļ��У������ڽ�ѹ��ɺ�ɾ��ԭѹ���ļ���
	static HttpDownloadNode* createLua(const char* url, int onNotify, const char* savePathName = "", unsigned int bytes = 0,
		const char* md5Val = "", const char* extractToFolder = "", bool noProgress = false);

	static HttpDownloadNode* create(const char* url, cocos2d::Ref* pTarget, SEL_CallFuncUDU selector,
		const char* savePathName = "", unsigned int bytes = 0,
		const char* md5Val = "", const char* extractToFolder = "", bool noProgress = false);

	// ֹͣ����Զ��Ӹ��ڵ����Ƴ�����
	void stop();

	virtual void update(float delta);
};

}

#endif /* __CC_HTTP_DOWNLOAD_NODE_H_ */
