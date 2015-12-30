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
#define to_tstring to_wstring
#define To_String std::to_wstring
#else
	typedef string tstring;
	typedef stringstream tstringstream;
#define to_tstring to_string
#define To_String std::to_string
#endif
};


CAMSNETLIB void WaitAndCloseHandle(HANDLE& hnd);
