#ifndef __UTILS_H__
#define __UTILS_H__
#include "cocos2d.h"
#include <string>
#include "base/allocator/CCAllocatorMacros.h"
#include "base/allocator/CCAllocatorMutex.h"

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
#include "platform/android/jni/JniHelper.h"
std::string jstring2utf8(jstring js, JNIEnv * env);
std::string getAndroidMainClassPath();
#endif

void sendBugReport(bool fromLua = false);
void recursiveSetShaderFunc(cocos2d::Node* pNode, const char* shaderKey,
	const char* vShader, const char* fShader);

// 解密文件内容。所返回的新内容为malloc所出。
unsigned char* decryptFileContent(unsigned char* fileContent, size_t& size);

namespace ccutil {

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	std::wstring StringUtf8ToWideChar(const std::string& strUtf8);
	std::string StringWideCharToUtf8(const std::wstring& strWideChar);
#endif

	FILE* fopenUtf8(const char* filename, const char* mode);
	
	class Utils{
	public:

		static std::string a2u(const char* gbkText);
		static std::string u2a(const char* utf8Text);
		static std::string md5(const char* str, int len);

		static std::string urlencode(const std::string& sourceStr);
		static std::string urldecode(const std::string& sourceStr);

		static std::string binary2hex(const char* binary, int len);

		// 是否启动跳帧模式？此模式将通过CCNode属性的变化，设置本帧是否需要重绘的标志，以自动跳过不需要重绘的帧绘制，从而减少耗电量。
		static void setFrameSkipping(bool enableFrameSkipping);

		// skip this frame
		static void setRedrawThisFrame(bool redrawThisFrame);

		// 退出程序
		static void quitApp();		// tolua_export

		// 为一个CCNode设定CCGLProgram
		static void setCCGLProgram(cocos2d::Node* pNode, const char* shaderKey, const char* vShader, const char* fShader);

		static void recursiveSetShader(cocos2d::Node* pNode, const char* shaderKey, const char* vShader, const char* fShader);

		// 在文件中进行log。
		static void log(const char* line);

		// 创建文件夹，可以同时创建不存在的多个父文件夹以及子文件夹。必须以/结尾，否则最后一个文件夹不会被创建。
		static void makedirs(const char* dirs);

		static std::string getExeFolder();

		static std::string getBestWritableFolder();

		// 文件是否存在
		static bool isFileExist(const char* filename);

		// 提供给lua使用的CCImage创建方法。需要delete
		static cocos2d::Image* newCCImageWithImageData(const char * pData, int nDataLen);

		// 获取时间（毫秒为单位）。只能用作时间比较之用。
		static double getTickCount();

		static void crash();

		static void setBugHandlerUserID(unsigned int userID);

		// 二维码生成。
		static std::string qrencodeToString(const char* s, int version = 0, int level = 2, int hint = 2, int casesensitive = 1, int margin = 1);	// tolua_export

		static cocos2d::Texture2D* qrencodeToTexture(const char* s, int size, cocos2d::Color4B black, cocos2d::Color4B white, int version = 0, int level = 2, int hint = 2, int casesensitive = 1, int margin = 1);		// tolua_export

		static std::string getInfoJson(const char* luaStack);

		static void sendBugReportLua(const char* luaErrMsg);

		static cocos2d::Rect getNodeRectInContainer(cocos2d::Node* pNode, cocos2d::Node* pContainer);
		static bool getIsNodeReallyVisible(cocos2d::Node* node);

		static void recursiveEaseFade(cocos2d::Node* sp, float t, float from, float to, float e);

		static std::string getExeName();

		static cocos2d::LabelTTF* createLabelWithStroke(const char *s, const char* fontName, int fontSize,
			int alignment, int vertAlignment, cocos2d::Size dimensions, cocos2d::Color3B fontFillColor,
			bool strokeEnabled, cocos2d::Color3B strokeColor, float strokeSize);

		static bool isCCLayer(cocos2d::Node* pNode);
		static bool isCCScrollView(cocos2d::Node* pNode);

		static bool setNearestFilter(bool b);

		// 清理所有数据并重新加载。
		static void switchGame(const char* name);

		// 返回上次switchGame中传入的参数。
		static std::string curGame();

		//userdefault读解密
		static std::string secureRead(const char* key, const char* def);
		//userdefault写加密
		static void secureWrite(const char* key, const char* s);
		// 获取当前运行的平台。为win32, android, ios之一
		static std::string platform();

		static void rmdir(const char* pathname, bool deleteChildrenOnly = true);

		static bool debugging();

		static std::string getLuaType(cocos2d::Ref* obj);

		static std::string getCPPTypeName(cocos2d::Ref* obj);

		static void setUIButtonDarken(bool b);

		// to必须是绝对路径。from可以是apk中的assets路径。
		static void copyFileTo(const char* from, const char* to);			// tolua_export

		static cocos2d::Size setWin32FrameSize(int w, int h);

		// 获得引擎版本，形如1.6.2.2451，即 主版本.次版本.修订号.编译次数。
		static std::string getEngineVersion();
	};

};

#endif // #ifndef __UTILS_H__