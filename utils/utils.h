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

// �����ļ����ݡ������ص�������Ϊmalloc������
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

		// �Ƿ�������֡ģʽ����ģʽ��ͨ��CCNode���Եı仯�����ñ�֡�Ƿ���Ҫ�ػ�ı�־�����Զ���������Ҫ�ػ��֡���ƣ��Ӷ����ٺĵ�����
		static void setFrameSkipping(bool enableFrameSkipping);

		// skip this frame
		static void setRedrawThisFrame(bool redrawThisFrame);

		// �˳�����
		static void quitApp();		// tolua_export

		// Ϊһ��CCNode�趨CCGLProgram
		static void setCCGLProgram(cocos2d::Node* pNode, const char* shaderKey, const char* vShader, const char* fShader);

		static void recursiveSetShader(cocos2d::Node* pNode, const char* shaderKey, const char* vShader, const char* fShader);

		// ���ļ��н���log��
		static void log(const char* line);

		// �����ļ��У�����ͬʱ���������ڵĶ�����ļ����Լ����ļ��С�������/��β���������һ���ļ��в��ᱻ������
		static void makedirs(const char* dirs);

		static std::string getExeFolder();

		static std::string getBestWritableFolder();

		// �ļ��Ƿ����
		static bool isFileExist(const char* filename);

		// �ṩ��luaʹ�õ�CCImage������������Ҫdelete
		static cocos2d::Image* newCCImageWithImageData(const char * pData, int nDataLen);

		// ��ȡʱ�䣨����Ϊ��λ����ֻ������ʱ��Ƚ�֮�á�
		static double getTickCount();

		static void crash();

		static void setBugHandlerUserID(unsigned int userID);

		// ��ά�����ɡ�
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

		// �����������ݲ����¼��ء�
		static void switchGame(const char* name);

		// �����ϴ�switchGame�д���Ĳ�����
		static std::string curGame();

		//userdefault������
		static std::string secureRead(const char* key, const char* def);
		//userdefaultд����
		static void secureWrite(const char* key, const char* s);
		// ��ȡ��ǰ���е�ƽ̨��Ϊwin32, android, ios֮һ
		static std::string platform();

		static void rmdir(const char* pathname, bool deleteChildrenOnly = true);

		static bool debugging();

		static std::string getLuaType(cocos2d::Ref* obj);

		static std::string getCPPTypeName(cocos2d::Ref* obj);

		static void setUIButtonDarken(bool b);

		// to�����Ǿ���·����from������apk�е�assets·����
		static void copyFileTo(const char* from, const char* to);			// tolua_export

		static cocos2d::Size setWin32FrameSize(int w, int h);

		// �������汾������1.6.2.2451���� ���汾.�ΰ汾.�޶���.���������
		static std::string getEngineVersion();
	};

};

#endif // #ifndef __UTILS_H__