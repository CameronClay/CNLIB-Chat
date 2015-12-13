//Copyright (c) <2015> <Cameron Clay>

#pragma once
#include <WinSock2.h>
#include <string>
#include <sstream>

#pragma comment(lib,"Ws2_32.lib")


#ifdef TCPCS_EXPORTS
#define CAMSNETLIB __declspec(dllexport)
#else
#define CAMSNETLIB __declspec(dllimport)
#endif

typedef WCHAR LIB_TCHAR;

namespace std
{
#ifdef UNICODE
	typedef wstring tstring;
	typedef wstringstream tstringstream;
#else
	typedef string tstring;
	typedef stringstream tstringstream;
#endif
};

#ifdef UNICODE
#define To_String std::to_wstring
#else
#define To_String std::to_string
#endif


#ifdef UNICODE
#define GetHostName(name, namelen)\
	char host[256];\
	gethostname(host, 256);\
	MultiByteToWideChar(CP_ACP, 0, host, 256, name, namelen)
#define Inet_ntot(inaddr, dest)\
	char* addr = inet_ntoa(inaddr);\
	MultiByteToWideChar(CP_ACP, 0, addr, strlen(addr), dest, strlen(addr))
#else
#define GetHostName gethostname
#define Inet_ntot(inaddr, dest);\
	char* name = inet_ntoa(inaddr);\
	memcpy(dest, name, sizeof(char) * strlen(name))
#endif

CAMSNETLIB void WaitAndCloseHandle(HANDLE& hnd);
