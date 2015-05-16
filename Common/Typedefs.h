#pragma once
#include <WinSock2.h>
#include <Windows.h>
#include <string>

#pragma comment(lib,"Ws2_32.lib")

namespace std
{
#ifdef UNICODE
	typedef wstring tstring;
#else
	typedef string tstring;
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

void WaitAndCloseHandle(HANDLE& hnd);
