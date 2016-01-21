#include "utils.h"
#include <sstream>
#include "cocos2d.h"
extern "C" {
#include "utils/md5.h"
};
#include <algorithm>
#include <sys/stat.h>
#if CC_TARGET_PLATFORM==CC_PLATFORM_IOS
#include <sys/types.h>
#include <errno.h>
#include "../iosshared/SDKUtils.h"
#endif
#if CC_TARGET_PLATFORM==CC_PLATFORM_WIN32
#include <io.h>
#include <direct.h>
#else
#include <iconv.h>
#endif
#include <sstream>
#include <iomanip>
//#include <pthread.h>
#include "extensions/cocos-ext.h"

#include "utils/Rijndael.h"

USING_NS_CC;
USING_NS_CC_EXT;


FILE* ccutil::fopenUtf8(const char* filename, const char* mode)
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	FILE* f = _wfopen(ccutil::StringUtf8ToWideChar(filename).c_str(),
		ccutil::StringUtf8ToWideChar(mode).c_str());
#else
	FILE* f = fopen(filename, mode);
#endif
	return f;
}

void recursiveSetShaderFunc(cocos2d::Node* pNode, const char* shaderKey,
	const char* vShader, const char* fShader)
{
	ccutil::Utils::recursiveSetShader(pNode, shaderKey, vShader, fShader);
}

#if (CC_TARGET_PLATFORM!=CC_PLATFORM_WIN32)

int code_convert(const char* from_charset, const char* to_charset, char* inbuf, size_t inlen, char* outbuf, size_t outlen)
{
	iconv_t cd;
	char* temp = inbuf;
	char** pin = &temp;
	char **pout = &outbuf;
	memset(outbuf, 0, outlen);
	int ret = -1;
	cd = iconv_open(to_charset, from_charset);
	if (cd)
	{
#if (CC_TARGET_PLATFORM==CC_PLATFORM_WIN32)
		if (iconv(cd, const_cast<const char**>(pin), &inlen, pout, &outlen) != -1)
#else
		if (iconv(cd, pin, &inlen, pout, &outlen) != -1)
#endif
		{
			ret = 0;
		}
		iconv_close(cd);
	}
	return ret;
}
#endif

std::string ccutil::Utils::a2u(const char* inbuf)
{
	std::string strRet;
	if (inbuf)
	{
		size_t inlen = strlen(inbuf);
		if (inlen)
		{
			char* outbuf = new char[inlen * 2 + 2];
#if CC_TARGET_PLATFORM == CC_PLATFORM_WIN32
			std::vector<wchar_t> wbuf(inlen * 2 + 2, 0);
			int wc = MultiByteToWideChar(936, 0, inbuf, inlen, &wbuf[0], inlen*2+2);
			int cc = WideCharToMultiByte(CP_UTF8, NULL, &wbuf[0], wc, outbuf, inlen * 2 + 2, NULL, NULL);
			outbuf[cc] = 0;
			strRet = outbuf;
#else
			if (code_convert("gbk", "utf-8", const_cast<char*>(inbuf), inlen, outbuf, inlen * 2 + 2) == 0)
			{
				strRet = outbuf;
			}
#endif
			delete[] outbuf;
		}
	}
	return strRet;
}

std::string ccutil::Utils::u2a(const char* inbuf)
{
	std::string strRet;
	if (inbuf)
	{
		size_t inlen = strlen(inbuf);
		if (inlen)
		{
			char* outbuf = new char[inlen * 2 + 2];
#if CC_TARGET_PLATFORM == CC_PLATFORM_WIN32
			std::vector<wchar_t> wbuf(inlen * 2 + 2, 0);
			int wc = MultiByteToWideChar(CP_UTF8, 0, inbuf, inlen, &wbuf[0], inlen*2+2);
			int cc = WideCharToMultiByte(936, NULL, &wbuf[0], wc, outbuf, inlen * 2 + 2, NULL, NULL);
			outbuf[cc] = 0;
			strRet = outbuf;
#else
			if (code_convert("utf-8", "gbk", const_cast<char*>(inbuf), inlen, outbuf, inlen * 2 + 2) == 0)
			{
				strRet = outbuf;
			}
#endif
			delete[] outbuf;
		}
	}
	return strRet;
}

std::vector<unsigned short> u2w(const char* inbuf)
{
	std::vector<unsigned short> strRet;
	if (inbuf)
	{
		size_t inlen = strlen(inbuf);
		if (inlen)
		{
			unsigned short* outbuf = new unsigned short[inlen * 2 + 2];
#if CC_TARGET_PLATFORM == CC_PLATFORM_WIN32
			int wc = MultiByteToWideChar(CP_UTF8, 0, inbuf, inlen, (wchar_t*)outbuf, inlen * 2 + 2);
			outbuf[wc] = 0;
			strRet.assign(outbuf, outbuf + inlen * 2 + 2);
#else
			if (code_convert("utf-8", "utf-16", const_cast<char*>(inbuf), inlen, (char*)outbuf, inlen * 4 + 4) == 0)
			{
				strRet.assign(outbuf, outbuf + inlen * 2 + 2);
			}
#endif
			delete[] outbuf;
		}
	}
	return strRet;
}

std::string ccutil::Utils::md5(const char* str, int len)
{
	unsigned char digest[16] = { 0 };
	md5_buffer(str, len, digest);
	char digestStr[48] = { 0 };
	sprintf(digestStr, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
		digest[0], digest[1], digest[2], digest[3], digest[4], digest[5], digest[6], digest[7],
		digest[8], digest[9], digest[10], digest[11], digest[12], digest[13], digest[14], digest[15]
		);
	return digestStr;
}

char hex2ch(unsigned char c)
{
	if (c >= '0' && c <= '9')
	{
		return c - '0';
	}
	else if (c >= 'a' && c <= 'z')
	{
		return 10 + c - 'a';
	}
	else if (c >= 'A' && c <= 'Z')
	{
		return 10 + c - 'A';
	}
	return 0;
}

std::vector<char> hex2binaryImpl(const char* s, int len)
{
	std::vector<char> result;
	if (len % 2 != 0)
	{
		return result;
	}
	result.resize(len / 2);
	for (int i = 0; i<len; i += 2)
	{
		result[i / 2] = (hex2ch(s[i]) << 4) | (hex2ch(s[i + 1]));
	}
	return result;
}

std::string ccutil::Utils::binary2hex(const char* binary, int len)
{
	using namespace std;
	std::ostringstream strm;
	for (int i = 0; i<len; ++i)
	{
		strm << hex << setw(2) << setfill('0') << setiosflags(ios::uppercase) << (unsigned int)((unsigned char)(binary[i]));
	}
	return strm.str();
}

void ccutil::Utils::quitApp()
{
	Director::getInstance()->end();
#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
	exit(0);
#endif
}

// Reload GLProgram when app recovered from background
class CCGLProgramReloader : public Ref {
public:
	CCGLProgramReloader()
	{
		NotificationCenter::getInstance()->addObserver(this,
			SEL_CallFuncO(&CCGLProgramReloader::onComeToForeGround), EVENT_COME_TO_FOREGROUND, NULL);
	}
	~CCGLProgramReloader()
	{
		NotificationCenter::getInstance()->removeObserver(this, EVENT_COME_TO_FOREGROUND);
	}
	void onComeToForeGround(Ref*)
	{
		for (int i = 0, c = shaderRecords.size(); i<c; ++i)
		{
			ShaderRecord& sr = shaderRecords[i];
			GLProgram* pProgram = ShaderCache::getInstance()->getGLProgram(sr.shaderKey.c_str());
			if (pProgram)
			{
				loadGLProgram(pProgram, sr.vShader.c_str(), sr.fShader.c_str());
			}
		}
	}
public:
	static CCGLProgramReloader& instance()
	{
		static CCGLProgramReloader singleton;
		return singleton;
	}

	struct ShaderRecord {
		std::string shaderKey, vShader, fShader;
	};
	std::vector<ShaderRecord> shaderRecords;

	GLProgram* getProgram(const char* shaderKey, const char* vShader, const char* fShader)
	{
		GLProgram* pProgram = ShaderCache::getInstance()->getGLProgram(shaderKey);
		if (pProgram == NULL)
		{
			pProgram = new GLProgram();
			loadGLProgram(pProgram, vShader, fShader);
			ShaderCache::getInstance()->addGLProgram(pProgram, shaderKey);
			pProgram->release();

			ShaderRecord sr;
			sr.shaderKey = shaderKey;
			sr.vShader = vShader;
			sr.fShader = fShader;
			shaderRecords.push_back(sr);
		}
		return pProgram;
	}

	void loadGLProgram(GLProgram* pProgram, const char* vShader, const char* fShader)
	{
		pProgram->initWithByteArrays(vShader, fShader);
		pProgram->bindAttribLocation(GLProgram::ATTRIBUTE_NAME_POSITION, GLProgram::VERTEX_ATTRIB_POSITION);
		pProgram->bindAttribLocation(GLProgram::ATTRIBUTE_NAME_COLOR, GLProgram::VERTEX_ATTRIB_COLOR);
		pProgram->bindAttribLocation(GLProgram::ATTRIBUTE_NAME_TEX_COORD, GLProgram::VERTEX_ATTRIB_TEX_COORD);

		pProgram->link();
		pProgram->updateUniforms();

		CHECK_GL_ERROR_DEBUG();
	}
};

void ccutil::Utils::setCCGLProgram(cocos2d::Node* pNode, const char* shaderKey, const char* vShader, const char* fShader)
{
#ifndef GDI_IMPL
	GLProgram* pProgram = CCGLProgramReloader::instance().getProgram(shaderKey, vShader, fShader);
	if (pProgram)
	{
		pNode->setGLProgram(pProgram);
	}
#else
	pNode->m_shaderName = shaderKey;
#endif
}

void ccutil::Utils::recursiveSetShader(cocos2d::Node* pNode, const char* shaderKey,
	const char* vShader, const char* fShader)
{
	Vector<Node*>& children = pNode->getChildren();
	for (int i = 0, c = children.size(); i < c; ++ i)
	{
		Node* child = children.at(i);
		recursiveSetShader(child, shaderKey, vShader, fShader);
	}
	setCCGLProgram(pNode, shaderKey, vShader, fShader);
}

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
namespace ccutil{

	std::wstring StringUtf8ToWideChar(const std::string& strUtf8)
	{
		std::wstring ret;
		if (!strUtf8.empty())
		{
			int nNum = MultiByteToWideChar(CP_UTF8, 0, strUtf8.c_str(), -1, nullptr, 0);
			if (nNum)
			{
				WCHAR* wideCharString = new WCHAR[nNum + 1];
				wideCharString[0] = 0;

				nNum = MultiByteToWideChar(CP_UTF8, 0, strUtf8.c_str(), -1, wideCharString, nNum + 1);

				ret = wideCharString;
				delete[] wideCharString;
			}
			else
			{
				CCLOG("Wrong convert to WideChar code:0x%x", GetLastError());
			}
		}
		return ret;
	}

	std::string StringWideCharToUtf8(const std::wstring& strWideChar)
	{
		std::string ret;
		if (!strWideChar.empty())
		{
			int nNum = WideCharToMultiByte(CP_UTF8, 0, strWideChar.c_str(), -1, nullptr, 0, nullptr, FALSE);
			if (nNum)
			{
				char* utf8String = new char[nNum + 1];
				utf8String[0] = 0;

				nNum = WideCharToMultiByte(CP_UTF8, 0, strWideChar.c_str(), -1, utf8String, nNum + 1, nullptr, FALSE);

				ret = utf8String;
				delete[] utf8String;
			}
			else
			{
				CCLOG("Wrong convert to Utf8 code:0x%x", GetLastError());
			}
		}

		return ret;
	}
};
std::string exeFolder()
{
	wchar_t buf[MAX_PATH] = { 0 };
	GetModuleFileNameW(NULL, buf, MAX_PATH);
	std::string path = ccutil::StringWideCharToUtf8(buf);
	int pos = path.rfind('\\');
	path = path.substr(0, pos);
	return path;
}
std::string workingFolder()
{
	wchar_t buf[MAX_PATH] = { 0 };
	GetCurrentDirectoryW(sizeof(buf), buf);
	std::string path = ccutil::StringWideCharToUtf8(buf);
	return path;
}
#endif

void makeFileAccessible(const char* filename)
{
#if CC_TARGET_PLATFORM!=CC_PLATFORM_WIN32
	//chmod(filename, S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH);
	std::ostringstream strm;
	strm << "chmod 775 \"" << filename << "\"";
	system(strm.str().c_str());
#endif
}

bool ccutil::Utils::isFileExist(const char* filename)
{
	return FileUtils::getInstance()->isFileExist(
		FileUtils::getInstance()->fullPathForFilename(filename));
}

const char* getLogFileName()
{
	static std::string s;
	if (s.empty())
	{
#if CC_TARGET_PLATFORM==CC_PLATFORM_WIN32
		s = exeFolder() + "/"  "/game.log";
#else
		s = ccutil::Utils::getBestWritableFolder() + "/game.log";
#endif
	}
	return s.c_str();
}

FILE* getLogFile()
{
	static FILE* f = NULL;
	if (f == NULL)
	{
		ccutil::Utils::makedirs(getLogFileName());
		f = ccutil::fopenUtf8(getLogFileName(), "wb");
	}
	return f;
}

void ccutil::Utils::log(const char* line)
{
	FILE* f = getLogFile();
	time_t tt = time(NULL);
	struct tm *t = localtime(&tt);
	char buf[32] = { 0 };
	strftime(buf, sizeof(buf), "[%H:%M:%S] ", t);
	fwrite(buf, 1, strlen(buf), f);
	fwrite(line, 1, strlen(line), f);
	fwrite("\r\n", 1, 2, f);
	fflush(f);
}

bool createDirectory(const char *path)
{
#if (CC_TARGET_PLATFORM !=CC_PLATFORM_WIN32)
	mode_t processMask = umask(0);
	int ret = mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO);
	umask(processMask);
	if (ret != 0 && (errno != EEXIST))
	{
		return false;
	}

	return true;
#else
	BOOL ret = CreateDirectoryA(path, NULL);
	if (!ret && ERROR_ALREADY_EXISTS != GetLastError())
	{
		return false;
	}
	return true;
#endif
}

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
std::string jstring2utf8(jstring js, JNIEnv * env)
{
	jboolean isCopy;
	const char* s = env->GetStringUTFChars(js, &isCopy);
	std::string ss = s;
	env->ReleaseStringUTFChars(js, s);
	return ss;
}

std::string getAndroidMainClassPath()
{
	static std::string pkgname;
	if (pkgname.empty())
	{
		JniMethodInfo minfo;
		bool isHave = JniHelper::getStaticMethodInfo(minfo, "com/lkgame/PackageInfos", "getMainClassPath",
			"()Ljava/lang/String;");
		if (isHave)
		{
			pkgname = jstring2utf8((jstring)minfo.env->CallStaticObjectMethod(minfo.classID, minfo.methodID), minfo.env);
		}
	}
	return pkgname;
}

std::string ccutil::Utils::getBestWritableFolder()
{
	static std::string bwf;
	if (bwf.empty())
	{
		JniMethodInfo minfo;
		bool isHave = JniHelper::getStaticMethodInfo(minfo, "com/lkgame/SDKCallback", "getWritableFolder",
			"()Ljava/lang/String;");
		if (isHave)
		{
			bwf = jstring2utf8((jstring)minfo.env->CallStaticObjectMethod(minfo.classID, minfo.methodID), minfo.env);
		}
	}
	return bwf;
}

#elif (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
std::string ccutil::Utils::getBestWritableFolder()
{
	return "";
}
#elif (CC_TARGET_PLATFORM == CC_PLATFORM_IOS) || (CC_TARGET_PLATFORM==CC_PLATFORM_MAC)
std::string ccutil::Utils::getBestWritableFolder()
{
	std::string result = FileUtils::getInstance()->getWritablePath();
	if (!result.empty() && ((*result.rbegin()) == '\\' || (*result.rbegin()) == '/'))
	{
		result = result.substr(0, result.size() - 1);
	}
	return result;
}
#endif

bool fileOrDirExists(const char* filename)
{
#if CC_TARGET_PLATFORM==CC_PLATFORM_WIN32
	return GetFileAttributesA(filename) != INVALID_FILE_ATTRIBUTES;
#else
	struct stat st = { 0 };
	return stat(filename, &st) == 0;
#endif
}

void makeSureFileCanBeSaved(const char* path, bool enableLog)
{
#if CC_TARGET_PLATFORM==CC_PLATFORM_ANDROID
	std::vector<char> buf(path, path + strlen(path) + 1);
	char* dir = &buf[0];
	int c = buf.size() - 1;
	for (int i = c; i>0; --i)
	{
		if (buf[i] == '/' || buf[i] == '\\')
		{
			buf[i] = 0;
			break;
		}
	}
	JniMethodInfo t;
	bool isHave = JniHelper::getStaticMethodInfo(t, "com/lkgame/SDKCallback", "makedirs",
		"(Ljava/lang/String;)V");
	if (isHave)
	{
		jstring jdir = t.env->NewStringUTF(dir);
		t.env->CallStaticVoidMethod(t.classID, t.methodID, jdir);
	}
#else
	std::vector<char> buf(path, path + strlen(path) + 1);
	char* dir = &buf[0];
	int c = buf.size() - 1;
	for (int i = 1; i<c; ++i)
	{
		if (dir[i] == '/' || dir[i] == '\\')
		{
			dir[i] = 0;
			if (!fileOrDirExists(dir))
			{
				if (enableLog)
				{
					ccutil::Utils::log((std::string("creating ") + dir + "...").c_str());
					ccutil::Utils::log((std::string("folder = ") + path).c_str());
				}

				createDirectory(dir);
			}
			dir[i] = '/';
		}
	}
#endif
}

void ccutil::Utils::makedirs(const char* path)
{
	makeSureFileCanBeSaved(path, true);
}

Image* ccutil::Utils::newCCImageWithImageData(const char * pData, int nDataLen)
{
	Image* result = new Image();
	result->initWithImageData((unsigned char*)pData, nDataLen);
	return result;
}

void ccutil::Utils::crash()
{
	int* p = (int*)0;
	*p = 1;
}


#if CC_TARGET_PLATFORM==CC_PLATFORM_WIN32
static DWORD WINAPI doErrUpload(LPVOID lpThreadParameter)
{
	sendBugReport(true);
	return 0;
}
#else
void *doErrUpload(void *userdata)
{
	sendBugReport(true);
	return 0;
}
#endif

Rect ccutil::Utils::getNodeRectInContainer(Node* pNode, Node* pContainer)
{
	Rect rect = Rect(0, 0, pNode->getContentSize().width, pNode->getContentSize().height);
	rect = RectApplyAffineTransform(rect, pNode->getNodeToWorldAffineTransform());
	return RectApplyAffineTransform(rect, pContainer->getWorldToNodeAffineTransform());
}

bool ccutil::Utils::getIsNodeReallyVisible(Node* node)
{
	if (!node->isVisible()){
		return false;
	}

	for (Node *c = node->getParent(); c != NULL; c = c->getParent())
	{
		if (c->isVisible() == false)
		{
			return false;
		}
	}
	return true;
}

std::string ccutil::Utils::getExeName()
{
#if CC_TARGET_PLATFORM==CC_PLATFORM_WIN32
	return "";
#else
	return "";
#endif
}

bool ccutil::Utils::isCCLayer(Node* pNode)
{
	return dynamic_cast<Layer*>(pNode) != NULL;
}

bool ccutil::Utils::isCCScrollView(Node* pNode)
{
	return dynamic_cast<ScrollView*>(pNode) != NULL;
}


void ccutil::Utils::copyFileTo(const char* from, const char* to)
{
	ssize_t size = 0;
	unsigned char* data = cocos2d::FileUtils::getInstance()->getFileData(from, "rb", &size);
	if (data)
	{
		FILE* f = fopen(to, "wb");
		if (f)
		{
			unsigned int p = 0;
			while (size>0)
			{
				unsigned int written = fwrite((void*)(data + p), 1, size, f);
				if (written == 0)
				{
					break;
				}
				p = p + written;
				size = size - written;
			}
			fflush(f);
			fclose(f);
		}
		delete data;
	}
}

std::string ccutil::Utils::curGame()
{
	return "";
}

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
cocos2d::Size setWin32Size(int &winw, int &winh)
{
	// make sure window can be fit into work area
	GLView* glView = Director::getInstance()->getOpenGLView();
	if (glView)
	{
		HWND hwnd = glView->getWin32Window();
		RECT winRect = { 0 }, clientRect = { 0 };
		::GetWindowRect(hwnd, &winRect);
		::GetClientRect(hwnd, &clientRect);
		int dx = abs(winRect.right - winRect.left) - abs(clientRect.right - clientRect.left);
		int dy = abs(winRect.bottom - winRect.top) - abs(clientRect.bottom - clientRect.top);
		RECT workRect = { 0 };
		HWND parentHwnd = GetParent(hwnd);
		if (parentHwnd != NULL)
		{
			::GetClientRect(parentHwnd, &workRect);
		}
		else
		{
			SystemParametersInfoA(SPI_GETWORKAREA, 0, &workRect, 0);
		}
		int wx = workRect.right - workRect.left;
		int wy = workRect.bottom - workRect.top;
		if (winw>wx - dx)
		{
			int oldWinW = winw;
			winw = wx - dx;
			winh = winh*winw / oldWinW;
		}
		if (winh>wy - dy)
		{
			int oldWinH = winh;
			winh = wy - dy;
			winw = winw*winh / oldWinH;
		}

		glView->setFrameSize(winw, winh);

		::GetClientRect(hwnd, &clientRect);
		return cocos2d::Size(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
	}
	return cocos2d::Size(0,0);
}
#endif	// #if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)

cocos2d::Size ccutil::Utils::setWin32FrameSize(int w, int h)
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	return setWin32Size(w, h);
#else
	return cocos2d::Size(0,0);
#endif	// #if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
}

// 对key进行md5
//加密得到encrypt(s)
//  添加校验码encrypt(s)+checkCode
void ccutil::Utils::secureWrite(const char* key, const char* s)
{
	if (!key || !s)
	{
		return;
	}

	int sLen = strlen(s);//加密内容长度
	int keyLen = strlen(key);
	unsigned char signature1[] = { 54, 73, 5, 154, 243, 3 };
	unsigned char signature2[] = { 50, 72, 4, 150, 240, 5 };

	char* signedKey = (char*)malloc(keyLen + sizeof(signature1) + 1);
	memset(signedKey, '\0', keyLen + sizeof(signature1) + 1);
	memcpy(signedKey, key, keyLen);
	memcpy(signedKey + keyLen, signature1, sizeof(signature1));

	std::string keyMd5 = md5(signedKey, strlen(signedKey));// 对key进行md5

	const unsigned char encryptKey[16] = {
		0x7c, 0xf9, 0xda, 0x5b, 0xd9, 0x12, 0x3e, 0x60, 0x79, 0x67, 0xe1, 0xd4, 0x43, 0xd1, 0x84, 0x87
	};
	std::string strEncryptKey = md5((char*)encryptKey, 16);//AES加密密钥

	int totalLen = sLen;
	if (!(sLen % 16) == 0)
	{
		totalLen = (sLen / 16 + 1) * 16;//加密内容长度补全为16的倍数
	}

	void* srcData = malloc(totalLen);
	void* outData = malloc(totalLen);
	memset(srcData, '\0', totalLen);
	memset(outData, '\0', totalLen);
	memcpy(srcData, s, sLen);

	CRijndael Rijndael;
	Rijndael.MakeKey(strEncryptKey.c_str(), CRijndael::sm_chain0, 32, 16);
	Rijndael.Encrypt((const char*)srcData, (char*)outData, totalLen);// 采用rijndael对s进行AES加密enc(s)

	std::string hexResult = binary2hex((char*)outData, totalLen);

	char* strMode = (char*)malloc(keyLen + totalLen + sizeof(signature2) + 1);
	memset(strMode, '\0', keyLen + totalLen + sizeof(signature2) + 1);

	memcpy(strMode, key, keyLen);
	memcpy(strMode + keyLen, outData, totalLen);
	memcpy(strMode + keyLen + totalLen, signature2, sizeof(signature2));//key +outData+signature

	std::string checkMd5 = md5(strMode, strlen(strMode));// 添加校验 md5(key +outData+signature)

	char checkCode[16] = { 0 };
	for (int i = 0; i < 15; i++)
	{
		checkCode[i] = checkMd5[i * 2];;//取md5字符串的奇数位字符作为校验码
	}

	std::string strValue = hexResult + checkCode;// 添加校验码chkcode

	UserDefault::getInstance()->setStringForKey(keyMd5.c_str(), strValue);// 将 enc(s)+checkCode 写入UserDefaults，键为md5(key)

	free(signedKey);
	free(srcData);
	free(outData);
	free(strMode);
	strMode = NULL;
}

// 对key进行md5
// 取出 encrypt(s)+checkCode
// 解密encrypt(s)得到s
// 通过检验码进行校验，校验失败则返回def
// 返回s
std::string ccutil::Utils::secureRead(const char* key, const char* def)
{
	if (!key)
	{
		return "";
	}

	int keyLen = strlen(key);
	unsigned char signature1[] = { 54, 73, 5, 154, 243, 3 };
	unsigned char signature2[] = { 50, 72, 4, 150, 240, 5 };

	char* signedKey = (char*)malloc(keyLen + sizeof(signature1) + 1);
	memset(signedKey, '\0', keyLen + sizeof(signature1) + 1);
	memcpy(signedKey, key, keyLen);
	memcpy(signedKey + keyLen, signature1, sizeof(signature1));

	std::string keyMd5 = md5(signedKey, strlen(signedKey));// 对key进行md5

	std::string encryptValue = UserDefault::getInstance()->getStringForKey(keyMd5.c_str());// 取出 enc(s)+chksum

	const unsigned char encryptKey[16] = {
		0x7c, 0xf9, 0xda, 0x5b, 0xd9, 0x12, 0x3e, 0x60, 0x79, 0x67, 0xe1, 0xd4, 0x43, 0xd1, 0x84, 0x87
	};
	std::string strEncryptKey = md5((char*)encryptKey, 16);//aes加密密钥

	int valueLen = encryptValue.length();
	if (valueLen == 0)
	{
		return def;
	}

	std::string strValue(encryptValue, 0, valueLen - 15);// 取出 enc(s)去掉校验码
	std::string strCheck(encryptValue, valueLen - 15, valueLen);

	std::vector<char> binaryData = hex2binaryImpl(strValue.c_str(), strValue.length());

	int totalLen = binaryData.size();

	void* srcData = malloc(totalLen);
	void* outData = malloc(totalLen + 1);
	memset(srcData, '\0', totalLen);
	memset(outData, '\0', totalLen + 1);
	memcpy(srcData, &binaryData[0], totalLen);

	CRijndael Rijndael;// 对enc(s)进行解密
	Rijndael.MakeKey(strEncryptKey.c_str(), CRijndael::sm_chain0, 32, 16);
	Rijndael.Decrypt((const char*)srcData, (char*)outData, totalLen);
	std::string result((char*)outData);

	char* strMode = (char*)malloc(keyLen + totalLen + sizeof(signature2) + 1);
	memset(strMode, '\0', keyLen + totalLen + sizeof(signature2) + 1);

	memcpy(strMode, key, keyLen);
	memcpy(strMode + keyLen, &binaryData[0], totalLen);
	memcpy(strMode + keyLen + totalLen, signature2, sizeof(signature2));

	std::string checkMd5 = md5(strMode, strlen(strMode));
	char checkCode[16] = { 0 };
	for (int i = 0; i < 15; i++)
	{
		checkCode[i] = checkMd5[i * 2];//取md5字符串的奇数位字符作为校验码
	}

	free(signedKey);
	free(srcData);
	free(outData);
	free(strMode);
	strMode = NULL;
	if (!strcmp(checkCode, strCheck.c_str()))//对比校验码
	{
		return result;
	}
	else{
		return def;
	}
}

std::string ccutil::Utils::platform()
{
#if (CC_TARGET_PLATFORM==CC_PLATFORM_ANDROID)
	return "android";
#elif (CC_TARGET_PLATFORM==CC_PLATFORM_IOS)
	return "ios";
#else
	return "win32";
#endif
}

#if CC_TARGET_PLATFORM!=CC_PLATFORM_WIN32
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>

int is_dir(char * filename)
{
	struct stat buf;
	int ret = stat(filename, &buf);
	if (0 == ret)
	{
		if (buf.st_mode & S_IFDIR)
		{
			//printf("%s is folder\n",filename);
			return 0;
		}
		else
		{
			//printf("%s is file\n",filename);
			return 1;
		}
	}
	return -1;
}

int delete_dir(const char * dirname, bool deleteChildrenOnly)
{
	char chBuf[256];
	DIR * dir = NULL;
	struct dirent *ptr;
	int ret = 0;
	dir = opendir(dirname);
	if (NULL == dir)
	{
		return -1;
	}
	while ((ptr = readdir(dir)) != NULL)
	{
		ret = strcmp(ptr->d_name, ".");
		if (0 == ret)
		{
			continue;
		}
		ret = strcmp(ptr->d_name, "..");
		if (0 == ret)
		{
			continue;
		}
		snprintf(chBuf, 256, "%s/%s", dirname, ptr->d_name);
		ret = is_dir(chBuf);
		if (0 == ret)
		{
			//printf("%s is dir\n", chBuf);
			ret = delete_dir(chBuf, false);
			if (0 != ret)
			{
				return -1;
			}
		}
		else if (1 == ret)
		{
			//printf("%s is file\n", chBuf);
			ret = remove(chBuf);
			if (0 != ret)
			{
				return -1;
			}
		}
	}
	closedir(dir);

	if (!deleteChildrenOnly)
	{
		ret = remove(dirname);
		if (0 != ret)
		{
			return -1;
		}
	}
	return 0;
}

void ccutil::Utils::rmdir(const char* pathname, bool deleteChildrenOnly /*= false*/)
{
	delete_dir(pathname, deleteChildrenOnly);
}
#else
#include <sys/stat.h>

BOOL DeleteDirectory(const wchar_t* lpszDir, bool deleteChildrenOnly)
{
	if (NULL == lpszDir || L'\0' == lpszDir[0])
	{
		return FALSE;
	}

	if (GetFileAttributesW(lpszDir)==INVALID_FILE_ATTRIBUTES)
	{
		return FALSE;
	}

	WIN32_FIND_DATAW wfd = { 0 };
	wchar_t szFile[MAX_PATH] = { 0 };
	wchar_t szDelDir[MAX_PATH] = { 0 };

	lstrcpyW(szDelDir, lpszDir);
	if (lpszDir[lstrlenW(lpszDir) - 1] != L'\\')
	{
		_snwprintf(szDelDir, _countof(szDelDir) - 1, L"%s\\", lpszDir);
	}
	else
	{
		wcsncpy(szDelDir, lpszDir, _countof(szDelDir) - 1);
	}

	_snwprintf(szFile, _countof(szFile) - 1, L"%s*.*", szDelDir);
	HANDLE hFindFile = FindFirstFileW(szFile, &wfd);
	if (INVALID_HANDLE_VALUE == hFindFile)
	{
		return FALSE;
	}

	do
	{
		if (lstrcmpiW(wfd.cFileName, L".") == 0 || lstrcmpiW(wfd.cFileName, L"..") == 0)
		{
			continue;
		}

		_snwprintf(szFile, _countof(szFile) - 1, L"%s%s", szDelDir, wfd.cFileName);
		if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			DeleteDirectory(szFile, false);
		}
		else
		{
			DeleteFile(szFile);
		}

	} while (FindNextFile(hFindFile, &wfd));

	FindClose(hFindFile);
	if (!deleteChildrenOnly)
	{
		RemoveDirectory(szDelDir);
	}
	return TRUE;
}

void ccutil::Utils::rmdir(const char* pathname, bool deleteChildrenOnly /*= false*/)
{
	std::vector<unsigned short> wpathname = u2w(pathname);
	DeleteDirectory((wchar_t*)&wpathname[0], deleteChildrenOnly);
}

#endif

std::string ccutil::Utils::getExeFolder()
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	return exeFolder();
#else
	return "";
#endif
}

void ccutil::Utils::recursiveEaseFade(Node* sp, float t, float from, float to, float e)
{
	Vector<Node*>& children = sp->getChildren();
	int count = children.size();
	for (int i = 0; i<count; ++i)
	{
		Node* child = dynamic_cast<Node*>(children.at(i));
		if (child)
		{
			recursiveEaseFade(child, t, from, to, e);
		}
	}

	float opacity = sp->getOpacity();
	sp->setOpacity(from*opacity / 255);
	sp->runAction(EaseOut::create(FadeTo::create(t, to*opacity / 255), e));
}

LabelTTF* ccutil::Utils::createLabelWithStroke(const char *s, const char* fontName, int fontSize, /*TextAlignment*/int alignment, /*VerticalTextAlignment*/int vertAlignment, Size dimensions, Color3B fontFillColor, bool strokeEnabled, Color3B strokeColor, float strokeSize)
{
	FontDefinition fd;
	fd._fontName = fontName;
	fd._fontSize = fontSize;
	fd._alignment = (TextHAlignment)alignment;
	fd._vertAlignment = (TextVAlignment)vertAlignment;
	fd._dimensions = dimensions;
	fd._fontFillColor = fontFillColor;
	fd._shadow._shadowEnabled = false;
	fd._shadow._shadowOffset = Size(0,0);
	fd._shadow._shadowBlur = 0;
	fd._shadow._shadowOpacity = 0;
	fd._stroke._strokeEnabled = strokeEnabled;
	fd._stroke._strokeColor = strokeColor;
	fd._stroke._strokeSize = strokeSize;
	LabelTTF* label = LabelTTF::createWithFontDefinition(s, fd);
	return label;
}

bool ccutil::Utils::debugging()
{
#ifdef _DEBUG
	return true;
#else
	return false;
#endif
}

std::string ccutil::Utils::getCPPTypeName(cocos2d::Ref* obj)
{
	return typeid(*obj).name();
}

unsigned char* decryptFileContent(unsigned char* fileContent, size_t& size)
{
	return fileContent;
}

//#ifndef NO_ICONV
//
//
//#endif	//#ifndef NO_ICONV
//
//#include "Cooperation.h"
//
//void cooperationLogin(bool usesTestServer, LUA_FUNCTION callback, const char* spreader, bool isChangeAount, const char* openVoucherashx, bool autoTry)
//{
//	Cooperation::instance().CooperationLogin(usesTestServer, callback, spreader, isChangeAount, openVoucherashx, autoTry);
//}
//
//void cooperationUpdateFullApk(LUA_FUNCTION callback)
//{
//	Cooperation::instance().CooperationUpdateFullApk(callback);
//}
//
//bool cooperationRecharge(int price, std::string payString, std::string type)
//{
//	return Cooperation::instance().CooperationRecharge(price, payString, type);
//}
//
//bool cooperationShare(std::string title, std::string content, std::string imagePath, std::string url, int platform)
//{
//	return Cooperation::instance().CooperationShare(title, content, imagePath, url, platform);
//}
//
//bool cooperationRegularize()
//{
//	return Cooperation::instance().CooperationRegularize();
//}
//
//bool cooperationStartAppMsgPush(std::string aount)
//{
//	return Cooperation::instance().CooperationStartAppMsgPush(aount);
//}
//
//bool cooperationEnableAppMsgPush(std::string tageName, bool enabled)
//{
//	return Cooperation::instance().CooperationEnableAppMsgPush(tageName, enabled);
//}
//
//#include "LKWebAuthen.h"
//
//std::string LKWebEncryptLua(char* content)
//{
//	char buf[257] = { 0 };
//	LKWebEncrypt(content, buf, sizeof(buf));
//	return buf;
//}
//
//std::string LKRSAEncryptLua( /*IN*/ const char* publicKey, /*IN*/ const char* content)
//{
//	char buf[257] = { 0 };
//	try
//	{
//		LKRSAEncrypt(publicKey, content, buf, sizeof(buf));
//	}
//	catch (...)
//	{
//		return "";
//	}
//	return buf;
//}
//
//std::string LKRSADecryptLua( /*IN*/ const char* privateKey, /*IN*/ const char* encrypted)
//{
//	char buf[129] = { 0 };
//	try
//	{
//		LKRSADecrypt(privateKey, encrypted, buf, sizeof(buf));
//	}
//	catch (...)
//	{
//		return "";
//	}
//	return buf;
//}
//
//lua_Object LKAESEncryptLua( /*IN*/ const char* key, /*IN*/ int keyLen, /*IN*/ const char* content, /*IN*/ int contentLen)
//{
//	cocos2d::LuaStack* pStack = ((cocos2d::LuaEngine*)cocos2d::ScriptEngineManager::sharedManager()
//		->getScriptEngine())->getLuaStack();
//
//	std::vector<unsigned char> buf(contentLen, 0);
//	LKAESEncrypt((const unsigned char*)key, keyLen, (const unsigned char*)content, contentLen, &buf[0]);
//	lua_pushlstring(pStack->getLuaState(), (const char*)(&buf[0]), contentLen);
//	return -1;
//}
//
//lua_Object LKAESDecryptLua( /*IN*/ const char* key, /*IN*/ int keyLen, /*IN*/ const char* encrypted, /*IN*/ int encryptedLen)
//{
//	cocos2d::LuaStack* pStack = ((cocos2d::LuaEngine*)cocos2d::ScriptEngineManager::sharedManager()
//		->getScriptEngine())->getLuaStack();
//
//	std::vector<unsigned char> buf(encryptedLen, 0);
//	LKAESDecrypt((const unsigned char*)key, keyLen, (const unsigned char*)encrypted, encryptedLen, &buf[0]);
//	lua_pushlstring(pStack->getLuaState(), (const char*)(&buf[0]), encryptedLen);
//	return -1;
//}
//
//// Reload GLProgram when app recovered from background
//class GLProgramReloader : public Object {
//public:
//	GLProgramReloader()
//	{
//		NotificationCenter::sharedNotificationCenter()->addObserver(this,
//			SEL_CallFuncO(&GLProgramReloader::onComeToForeGround), EVENT_COME_TO_FOREGROUND, NULL);
//	}
//	~GLProgramReloader()
//	{
//		NotificationCenter::sharedNotificationCenter()->removeObserver(this, EVENT_COME_TO_FOREGROUND);
//	}
//	void onComeToForeGround(Object*)
//	{
//		for (int i = 0, c = shaderRecords.size(); i<c; ++i)
//		{
//			ShaderRecord& sr = shaderRecords[i];
//			GLProgram* pProgram = ShaderCache::getInstance()->programForKey(sr.shaderKey.c_str());
//			if (pProgram)
//			{
//				loadGLProgram(pProgram, sr.vShader.c_str(), sr.fShader.c_str());
//			}
//		}
//	}
//public:
//	static GLProgramReloader& instance()
//	{
//		static GLProgramReloader singleton;
//		return singleton;
//	}
//
//	struct ShaderRecord {
//		std::string shaderKey, vShader, fShader;
//	};
//	std::vector<ShaderRecord> shaderRecords;
//
//	GLProgram* getProgram(const char* shaderKey, const char* vShader, const char* fShader)
//	{
//		GLProgram* pProgram = ShaderCache::getInstance()->programForKey(shaderKey);
//		if (pProgram == NULL)
//		{
//			pProgram = new GLProgram();
//			loadGLProgram(pProgram, vShader, fShader);
//			ShaderCache::getInstance()->addProgram(pProgram, shaderKey);
//			pProgram->release();
//
//			ShaderRecord sr;
//			sr.shaderKey = shaderKey;
//			sr.vShader = vShader;
//			sr.fShader = fShader;
//			shaderRecords.push_back(sr);
//		}
//		return pProgram;
//	}
//
//	void loadGLProgram(GLProgram* pProgram, const char* vShader, const char* fShader)
//	{
//		pProgram->initWithVertexShaderByteArray(vShader, fShader);
//		pProgram->addAttribute(kAttributeNamePosition, kVertexAttrib_Position);
//		pProgram->addAttribute(kAttributeNameColor, kVertexAttrib_Color);
//		pProgram->addAttribute(kAttributeNameTexCoord, kVertexAttrib_TexCoords);
//
//		pProgram->link();
//		pProgram->updateUniforms();
//
//		CHECK_GL_ERROR_DEBUG();
//	}
//};
//
//void setGLProgram(cocos2d::Node* pNode, const char* shaderKey, const char* vShader, const char* fShader)
//{
//#ifndef GDI_IMPL
//	GLProgram* pProgram = GLProgramReloader::instance().getProgram(shaderKey, vShader, fShader);
//	if (pProgram)
//	{
//		pNode->setShaderProgram(pProgram);
//	}
//#else
//	pNode->m_shaderName = shaderKey;
//#endif
//}
//
//std::string cooperationGetDeviceID()
//{
//	return Cooperation::getDeviceID();
//}
//
//std::string cooperationGetDeviceAttributes()
//{
//	return Cooperation::getDeviceAttributes();
//}
//
//#ifndef NO_STARTUP_PARAMS
//std::string getStartupParams()
//{
//#ifdef WIN32
//	static const char startup[] = "-startup=";
//	for (int i = 1; i<__argc; ++i)
//	{
//		const char* s = __argv[i];
//		if (s != NULL)
//		{
//			unsigned int startupLen = strlen(startup);
//			if (strlen(s)>startupLen && memcmp(s, startup, startupLen) == 0)
//			{
//				std::string stparams = s + startupLen;
//				std::string nextGame = Director::sharedDirector()->m_sNextGameName;
//				if (!nextGame.empty())
//				{
//					stparams = stparams.substr(0, stparams.size() - 1) + ",\"launch\":\""
//						+ AppDelegate::distConfig().mainModule + "\"" + stparams.substr(stparams.size() - 1);
//				}
//				return a2u(stparams.c_str());
//			}
//		}
//	}
//#endif
//	return "";
//}
//#endif
//
//MenuWithTouchRectEx* MenuWithTouchRectEx::create(cocos2d::Array* pArrayOfItems)
//{
//	MenuWithTouchRectEx* ret = new MenuWithTouchRectEx();
//	ret->initWithArray(pArrayOfItems);
//	ret->autorelease();
//	return ret;
//}
//
//MenuWithTouchRectEx* MenuWithTouchRectEx::create()
//{
//	MenuWithTouchRectEx* ret = new MenuWithTouchRectEx();
//	ret->init();
//	ret->autorelease();
//	return ret;
//}
//
//MenuWithTouchRectEx::MenuWithTouchRectEx()
//{
//	setPosition(0, 0);
//	setAnchorPoint(p(0, 0));
//	touchRect = RectMake(0, 0, 0, 0);
//}
//
//MenuWithTouchRectEx::~MenuWithTouchRectEx()
//{
//}
//
//void MenuWithTouchRectEx::setTouchRect(const cocos2d::Rect& touchRect)
//{
//	this->touchRect = touchRect;
//}
//
//bool MenuWithTouchRectEx::TouchBegan(Touch* touch, Event* event)
//{
//	if (!inTouchRect(touch))
//	{
//		return false;
//	}
//
//	return Menu::TouchBegan(touch, event);
//}
//
//bool MenuWithTouchRectEx::TouchHover(cocos2d::Touch *pTouch, cocos2d::Event *pEvent)
//{
//	if (!inTouchRect(pTouch))
//	{
//		if (g_pHoverItem)
//		{
//			Node* parent = g_pHoverItem->getParent();
//			if (parent && dynamic_cast<Menu*>(parent) == this)
//			{
//				g_pHoverItem->mouseout();
//				g_pHoverItem = NULL;
//			}
//		}
//		return false;
//	}
//
//	return Menu::TouchHover(pTouch, pEvent);
//}
//
//bool MenuWithTouchRectEx::inTouchRect(Touch* touch)
//{
//	Rect tr = touchRect;
//	if (tr.size.width == 0 && tr.size.height == 0)
//	{
//		// Find a ScrollView parent
//		Node* p = this->getParent();
//		while (p != NULL)
//		{
//			ScrollView* sv = dynamic_cast<ScrollView*>(p);
//			if (sv != NULL)
//			{
//				Size size = sv->getViewSize();
//				Point ap = p->getAnchorPoint();
//				Point pt1(0 - ap.x*size.width, 0 - ap.x*size.height),
//					pt2(size.width - ap.x*size.width, size.height - ap.x*size.height);
//				pt1 = p->convertToWorldSpace(pt1);
//				pt2 = p->convertToWorldSpace(pt2);
//				pt1 = this->convertToNodeSpace(pt1);
//				pt2 = this->convertToNodeSpace(pt2);
//
//				tr.origin = pt1;
//				tr.size.setSize(pt2.x - pt1.x, pt2.y - pt1.y);
//				break;
//			}
//			p = p->getParent();
//		}
//	}
//	if (tr.size.width != 0 && tr.size.height != 0)
//	{
//		Point pt = this->convertToNodeSpace(touch->getLocation());
//		if (!tr.containsPoint(pt))
//		{
//			return false;
//		}
//	}
//	return true;
//}
//
//bool setDeviceOrientation(int orientation, LUA_FUNCTION onRotated)
//{
//	return Cooperation::setDeviceOrientation(orientation, onRotated);
//}
//
//void setAnimationInterval(float interval)
//{
//	Director::sharedDirector()->setAnimationInterval(interval);
//}
//
//#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
//typedef __int64 int64;
//double winGetSysTickCount64()
//{
//	static LARGE_INTEGER TicksPerSecond = { 0 };
//	LARGE_INTEGER Tick;
//
//	if (!TicksPerSecond.QuadPart)
//	{
//		QueryPerformanceFrequency(&TicksPerSecond);
//	}
//
//	QueryPerformanceCounter(&Tick);
//
//	int64 Seconds = Tick.QuadPart / TicksPerSecond.QuadPart;
//	int64 LeftPart = Tick.QuadPart - (TicksPerSecond.QuadPart*Seconds);
//	double MillSeconds = ((double)LeftPart*1000.0) / TicksPerSecond.QuadPart;
//	double Ret = Seconds*1000.0 + MillSeconds;
//	_ASSERT(Ret>0);
//	return Ret;
//}
//#endif
//
//double getTickCount()
//{
//#ifdef _WIN32
//	return (double)winGetSysTickCount64();
//#else
//	//time_t curTime = time(NULL);
//	struct timeval tv;
//	gettimeofday(&tv, 0);
//	double msOfDay = (((double)tv.tv_sec) * 1000.0) + (((double)tv.tv_usec) / 1000.0);
//
//	static double lastMsOfDay = msOfDay;
//	static double delta = 0;
//
//	if (lastMsOfDay>msOfDay && (lastMsOfDay - msOfDay)>23 * 60 * 60 * 1000)
//	{
//		delta = delta + 24 * 60 * 60 * 1000;
//	}
//	lastMsOfDay = msOfDay;
//
//	return double(msOfDay + delta);
//#endif
//}
//
//void cooperationSetBatteryChangedCallback(LUA_FUNCTION callback)
//{
//	Cooperation::instance().setBatteryChangedCallback(callback);
//}
//
//void cooperationSetSignalLevelChangedCallback(LUA_FUNCTION callback)
//{
//	Cooperation::instance().setSignalLevelChangedCallback(callback);
//}
//
//std::string cooperationCreateRandomUUID()
//{
//	return Cooperation::createRandomUUID();
//}
//
//void cooperationInstallApp(const char* pathname)
//{
//	Cooperation::installApp(pathname);
//}
//
//bool cooperationStartApp(const char* packageName, const char* mainCls, const char* extraParams)
//{
//	return Cooperation::startApp(packageName, mainCls, extraParams);
//}
//
//LuaRef downloadAndInstallAppCallback;
//
//void cppDownloadAndInstallAppCallback(void* userdata, int eventType, void* data)
//{
//	if (downloadAndInstallAppCallback.getT() == 0)
//	{
//		return;
//	}
//	cocos2d::LuaStack* pStack = ((cocos2d::LuaEngine*)cocos2d::ScriptEngineManager::sharedManager()
//		->getScriptEngine())->getLuaStack();
//	switch (eventType)
//	{
//	case Updater::EVENT_DEPENDS_UPDATE_FAILED:
//		pStack->pushString("failed");
//		pStack->pushString((const char*)data);
//		pStack->executeFunctionByHandler(downloadAndInstallAppCallback.getT(), 2);
//		downloadAndInstallAppCallback.reset();
//		break;
//	case Updater::EVENT_DEPENDS_UPDATING:
//	{
//		Updater::DependsUpdatingData* dud = (Updater::DependsUpdatingData*)data;
//		pStack->pushString("downloading");
//		pStack->pushFloat(dud->updated);
//		pStack->executeFunctionByHandler(downloadAndInstallAppCallback.getT(), 2);
//	}
//	break;
//	case Updater::EVENT_DEPENDS_UPDATE_COMPLETE:
//	case Updater::EVENT_APP_DOWNLOAD_COMPLETE:
//		pStack->pushString("sueeded");
//		pStack->executeFunctionByHandler(downloadAndInstallAppCallback.getT(), 1);
//		downloadAndInstallAppCallback.reset();
//		break;
//	}
//}
//
//void downloadAndInstallApp(const char* appName, LUA_FUNCTION callback)
//{
//	downloadAndInstallAppCallback.reset(callback);
//	Updater::downloadAndInstallApp(appName, &cppDownloadAndInstallAppCallback, NULL);
//}
//
//void downloadAndInstallAppFromURL(const char* url, LUA_FUNCTION callback)
//{
//	downloadAndInstallAppCallback.reset(callback);
//	Updater::downloadAndInstallAppFromURL(url, &cppDownloadAndInstallAppCallback, NULL);
//}
//
//cocos2d::Menu* createMenuWithTouchRect()
//{
//	return MenuWithTouchRectEx::create();
//}
//
//void setTouchRectOfMenuWithTouchRect(cocos2d::Menu* menu, const cocos2d::Rect& touchRect)
//{
//	MenuWithTouchRectEx* pMWTR = dynamic_cast<MenuWithTouchRectEx*>(menu);
//	if (menu)
//	{
//		pMWTR->setTouchRect(touchRect);
//	}
//}
//
//void openURL(const char* url)
//{
//	Cooperation::openURL(url);
//}
//
//void restartApp()
//{
//	Cooperation::restartApp();
//}
//
//void vibrate(int milliseconds)
//{
//	Cooperation::vibrate(milliseconds);
//}
//
//LuaRef deviceMsgHandler;
//
//void setDeviceMsgHandler(LUA_FUNCTION callback)
//{
//	deviceMsgHandler.reset(callback);
//}
//
//
//void setfield(lua_State* L, const char* key, int value)
//{
//	lua_pushinteger(L, value);
//	lua_setfield(L, -2, key);
//}
//
//void setboolfield(lua_State* L, const char* key, int value)
//{
//	if (value<0)
//	{
//		return;
//	}
//	lua_pushboolean(L, value);
//	lua_setfield(L, -2, key);
//}
//
//#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
//class IconKeeper {
//public:
//	HICON hIcon;
//	IconKeeper()
//	{
//		hIcon = NULL;
//	}
//	~IconKeeper()
//	{
//		reset(NULL);
//	}
//	void reset(HICON hNewIcon)
//	{
//		if (hIcon != NULL)
//		{
//			DestroyIcon(hIcon);
//		}
//		hIcon = hNewIcon;
//	}
//};
//IconKeeper bigIcon;
//IconKeeper smallIcon;
//
//void loadResourceIcon(LPCSTR resourceName)
//{
//	HICON hBigIcon = LoadIcon(GetModuleHandle(NULL), resourceName);
//	if (hBigIcon)
//	{
//		bigIcon.reset(hBigIcon);
//		SendMessage(EGLView::sharedOpenGLView()->getHWnd(), WM_SETICON, ICON_BIG, (LPARAM)hBigIcon);
//	}
//	SendMessage(EGLView::sharedOpenGLView()->getHWnd(), WM_SETICON, ICON_SMALL, (LPARAM)hBigIcon);
//	smallIcon.reset(NULL);
//}
//
//#endif
//
//bool loadWin32Icon(const char* module)
//{
//#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
//	std::string iconFilename = module;
//	iconFilename += "/pic/win32.ico";
//	if (isFileExist(iconFilename.c_str()))
//	{
//		static std::string lastIconFile;
//		if (lastIconFile != iconFilename)
//		{
//			lastIconFile = iconFilename;
//
//			// decrypt file and save to temp folder
//			unsigned int size = 0;
//			unsigned char* pData = FileUtils::getInstance()->getFileData(iconFilename.c_str(), "rb", &size);
//			if (pData)
//			{
//				// save to temp folder
//				std::string s = exeFolder() + "/LKCPUpdate/win32.ico";
//				makeSureFileCanBeSaved(s.c_str());
//				HANDLE hFile = ::CreateFileA(s.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
//					FILE_ATTRIBUTE_NORMAL, NULL);
//				if (hFile != INVALID_HANDLE_VALUE)
//				{
//					DWORD written = 0;
//					::WriteFile(hFile, pData, size, &written, NULL);
//					::CloseHandle(hFile);
//				}
//
//				_SAFE_DELETE_ARRAY(pData);
//				pData = NULL;
//
//				HICON hBigIcon = (HICON)LoadImageA(NULL, s.c_str(), IMAGE_ICON,
//					GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_LOADFROMFILE | LR_SHARED);
//				if (hBigIcon)
//				{
//					bigIcon.reset(hBigIcon);
//					SendMessage(EGLView::sharedOpenGLView()->getHWnd(), WM_SETICON, ICON_BIG, (LPARAM)hBigIcon);
//				}
//				HICON hSmallIcon = (HICON)LoadImageA(NULL, s.c_str(), IMAGE_ICON,
//					GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_LOADFROMFILE | LR_SHARED);
//				if (hSmallIcon)
//				{
//					smallIcon.reset(hSmallIcon);
//					SendMessage(EGLView::sharedOpenGLView()->getHWnd(), WM_SETICON, ICON_SMALL, (LPARAM)hSmallIcon);
//				}
//
//				::DeleteFileA(s.c_str());
//			}
//		}
//		return true;
//	}
//#endif
//	return false;
//}
//
//std::string getAppID()
//{
//	return Cooperation::getAppID();
//}
//
//void flashWindow(bool bInvert)
//{
//#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
//	HWND hwndFocus = ::GetFocus();
//	HWND hwnd = EGLView::sharedOpenGLView()->getHWnd();
//	if (hwnd != hwndFocus && IsChild(hwnd, hwndFocus) == FALSE)
//	{
//		FlashWindow(hwnd, bInvert ? TRUE : FALSE);
//	}
//#endif
//}
//
//
//std::string cooperationGetExtraParams()
//{
//#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
//	JniMethodInfo t;
//	if (JniHelper::getStaticMethodInfo(t, "com/lkgame/SDKCallback", "getExtraParams", "()Ljava/lang/String;"))
//	{
//		jstring jresult = (jstring)t.env->CallStaticObjectMethod(t.classID, t.methodID);
//		std::string result = jstring2utf8(jresult, t.env);
//		return result;
//	}
//#endif
//	return "";
//}
//
//void cooperationGetUserPic(const char* path)
//{
//	Cooperation::instance().getUserPicture(path);
//}
//
//#include "PlayVideo.h"
//void* curPlayVideoId = 0;
//std::map<void*, LuaRef> playVideoCallbacks;
//
//void onPlayFinishedLua(void* id)
//{
//	if (playVideoCallbacks[id].getT())
//	{
//		LuaStack* pStack = ((LuaEngine*)(ScriptEngineManager::sharedManager()->getScriptEngine()))->getLuaStack();
//		pStack->executeFunctionByHandler(playVideoCallbacks[id].getT(), 0);
//
//		playVideoCallbacks[id].reset();
//	}
//	playVideoCallbacks.erase(id);
//}
//
//void playVideo(const char* path, LUA_FUNCTION onPlayFinished, int flag /*= 0*/)
//{
//	playVideoCallbacks[curPlayVideoId].reset(onPlayFinished);
//	playVideoEx(path, &onPlayFinishedLua, curPlayVideoId, flag);
//	curPlayVideoId = (void*)((intptr_t)curPlayVideoId + 1);
//}
//
//void cooperationSetRechargeCallback(LUA_FUNCTION t)
//{
//	Cooperation::instance().setRechargeCallback(t);
//}
//void cooperationSetRegularizeCallback(LUA_FUNCTION t)
//{
//	Cooperation::instance().setRegularizeCallback(t);
//}
//void cooperationSetShareCallback(LUA_FUNCTION t)
//{
//	Cooperation::instance().setShareCallback(t);
//}
//bool cooperationCheckAppInstalled(const char* path)
//{
//	return Cooperation::instance().checkAppInstalled(path);
//}
//
//bool cooperationSmsPay(int price, std::string payString, std::string type, std::string phoneNo)
//{
//	Log("cooperationSmsPay");
//	return Cooperation::instance().CooperationSmsPay(price, payString, type, phoneNo);
//}
//
//std::string CooperationGetPhoneNo()
//{
//	return Cooperation::instance().CooperationGetPhoneNo();
//}
//
//
//Rect getNodeRectInContainer(Node* pNode, Node* pContainer)
//{
//	Rect rect = RectMake(0, 0, pNode->getContentSize().width, pNode->getContentSize().height);
//	rect = RectApplyAffineTransform(rect, pNode->nodeToWorldTransform());
//	return RectApplyAffineTransform(rect, pContainer->worldToNodeTransform());
//}
//
//bool getIsNodeReallyVisible(Node* node)
//{
//	if (!node->isVisible()){
//		return false;
//	}
//
//	for (Node *c = node->getParent(); c != NULL; c = c->getParent())
//	{
//		if (c->isVisible() == false)
//		{
//			return false;
//		}
//	}
//	return true;
//}
//
//bool isRGBANode(cocos2d::Object* pNode)
//{
//	RGBAProtocol* protocol = dynamic_cast<RGBAProtocol*>(pNode);
//	return protocol != NULL;
//}
//
//void setRGBAOpacity(Object* pNode, float opacity)
//{
//	RGBAProtocol* protocol = dynamic_cast<RGBAProtocol*>(pNode);
//	if (protocol)
//	{
//		protocol->setOpacity(opacity);
//	}
//}
//
//float getRGBAOpacity(Object* pNode)
//{
//	RGBAProtocol* protocol = dynamic_cast<RGBAProtocol*>(pNode);
//	if (protocol)
//	{
//		return protocol->getOpacity();
//	}
//	return 1.0f;
//}
//
//void addAnimationsToCacheWithDict(Dictionary* dict)
//{
//	Assert(dict, "Dictionary can not be NULL");
//
//	Dictionary* animations = (Dictionary*)dict->objectForKey("animations");
//
//	if (animations == NULL) {
//		LOG("No animations were found in provided dictionary.");
//		return;
//	}
//
//	AnimationCache * aniCache = AnimationCache::sharedAnimationCache();
//	SpriteFrameCache *frameCache = SpriteFrameCache::sharedSpriteFrameCache();
//secretread
//	DictElement* pElement = NULL;
//	// 迭代每一个元素  key=>dic
//	DICT_FOREACH(animations, pElement)
//	{
//		const char* name = pElement->getStrKey();
//		Dictionary* animationDict = (Dictionary*)pElement->getObject();
//
//		float delayPerUnit = animationDict->valueForKey("delayPerUnit")->floatValue(); // 每帧间隔时间
//		//int loops = animationDict->valueForKey("loops")->intValue(); // 循环次数 (暂时无用)
//		//bool restoreOriginalFrame = animationDict->valueForKey("restoreOriginalFrame")->boolValue(); //播放完动画还原为初始帧(暂时无用)
//
//		Array* frameArray = (Array*)animationDict->objectForKey("frames");
//
//		if (frameArray == NULL) {
//			LOG("Animation '%s' found in dictionary without any frames - cannot add to animation cache.", name);
//			continue;
//		}
//
//		Array* arr = Array::createWithCapacity(frameArray->count());
//
//		Object* pObj = NULL;
//		ARRAY_FOREACH(frameArray, pObj)
//		{
//			Dictionary* entry = (Dictionary*)(pObj);
//
//			const char* spriteFrameName = entry->valueForKey("spriteframe")->getCString();
//			SpriteFrame *spriteFrame = frameCache->spriteFrameByName(spriteFrameName);
//
//			if (!spriteFrame) {
//				LOG("Animation '%s' refers to frame '%s' which is not currently in the SpriteFrameCache. This frame will not be added to the animation.", name, spriteFrameName);
//
//				continue;
//			}
//
//			//float delayUnits = entry->valueForKey("delayUnits")->floatValue(); // (暂时无用)
//
//			arr->addObject(spriteFrame);
//		}
//
//		Animation *ani = Animation::createWithSpriteFrames(arr, delayPerUnit);
//		if (NULL == ani){
//			LOG("crate animation error '%s'", name);
//			return;
//		}
//		aniCache->addAnimation(ani, name);
//	}
//}
//
//// 一个ani文件可以包含多个animation
//void addAnimationsToCacheWithFile(const char *aniFile)
//{
//	Assert(aniFile, "Invalid ani file name");
//
//	std::string path = FileUtils::getInstance()->fullPathForFilename(aniFile);
//	Dictionary* dict = Dictionary::createWithContentsOfFile(path.c_str());
//
//	addAnimationsToCacheWithDict(dict);
//}
//
//SpriteFrame* getSpriteFrameByAnimationIdx(const char* szAniName, unsigned int idx)
//{
//	Animation *ani = AnimationCache::sharedAnimationCache()->animationByName(szAniName);
//	AnimationFrame* frame = (AnimationFrame*)ani->getFrames()->objectAtIndex(idx);
//	return frame->getSpriteFrame();
//}
//
//
//
//bool isLayer(Node* pNode)
//{
//	return dynamic_cast<Layer*>(pNode) != NULL;
//}
//
//bool isScrollView(Node* pNode)
//{
//	return dynamic_cast<ScrollView*>(pNode) != NULL;
//}
//
//LuaRef keyStrokeHandler;
//
//void onKeyStroke(bool isKeyDown, unsigned int vKey)
//{
//	if (keyStrokeHandler.getT())
//	{
//		cocos2d::LuaStack* pStack = ((cocos2d::LuaEngine*)cocos2d::ScriptEngineManager::sharedManager()
//			->getScriptEngine())->getLuaStack();
//
//		pStack->pushBoolean(isKeyDown);
//		pStack->pushInt(vKey);
//		pStack->executeFunctionByHandler(keyStrokeHandler.getT(), 2);
//	}
//}
//
//void setKeyStrokeHandler(LUA_FUNCTION handler)
//{
//	keyStrokeHandler.reset(handler);
//}
//
