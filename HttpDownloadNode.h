
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

	// 下载某个地址的文件。下载完成后将自从从parent node中移除自身。
	// onNotify用以接收进度通知与结束通知: onNotify(eventType, data); 消息列表为：
	//     "connected", nil; "size", sizeInBytes; "filename", filename; "progress", sizeInBytes; 
	//     "failed", reason, msg; "succeeded", nil/content;    // content只在savePathName为空时发送。
	// savePathName将指定保存路径。请不要将多个下载指定为同一个保存路径，不然文件内容有可能是两个下载内容的混合，导致出错。
	// 不指定savePathName时，"succeeded"消息的第二个参数将是下载的HTTP内容。
	// 如果指定了savePathName, bytes，则将自动支持断点续传功能。断点续传信息文件将保存在savePathName+".info"中，并将在下载完成后删除。
	// 如果指定了bytes和/或md5Val，则将在下载完成后验证文件大小与md5值。
	// 如果指定了extractToFolder，将尝试按zip文件格式将所下载的文件解压到extractToFolder文件夹，并将在解压完成后删除原压缩文件。
	static HttpDownloadNode* createLua(const char* url, int onNotify, const char* savePathName = "", unsigned int bytes = 0,
		const char* md5Val = "", const char* extractToFolder = "", bool noProgress = false);

	static HttpDownloadNode* create(const char* url, cocos2d::Ref* pTarget, SEL_CallFuncUDU selector,
		const char* savePathName = "", unsigned int bytes = 0,
		const char* md5Val = "", const char* extractToFolder = "", bool noProgress = false);

	// 停止后会自动从父节点中移除自身。
	void stop();

	virtual void update(float delta);
};

}

#endif /* __CC_HTTP_DOWNLOAD_NODE_H_ */
