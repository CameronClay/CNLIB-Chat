#pragma once
#include "CNLIB\Messages.h"
#include "CNLIB\HeapAlloc.h"
#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <string>

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


static LIB_TCHAR* GetTime()
{
	const UINT buffSize = GetTimeFormat(LOCALE_USER_DEFAULT, LOCALE_USE_CP_ACP, NULL, NULL, NULL, 0);
	LIB_TCHAR* buffer = alloc<LIB_TCHAR>(buffSize);
	GetTimeFormat(LOCALE_USER_DEFAULT, LOCALE_USE_CP_ACP, NULL, NULL, buffer, buffSize);
	return buffer;
}

static LIB_TCHAR* FormatText(BYTE* dat, DWORD datBytes, std::tstring& name, UINT& nBytes, bool timeStamps)
{
	if(timeStamps)
	{
		LIB_TCHAR* time = GetTime();
		// 7 for [] <> :, 1 for null
		nBytes = ((_tcslen(time) + name.size() + 7 + 1) * sizeof(LIB_TCHAR)) + datBytes;
		char* msg = alloc<char>(nBytes);
		_stprintf((LIB_TCHAR*)msg, _T("[%s] <%s>: %s"), time, name.c_str(), dat);
		dealloc(time);
		return (LIB_TCHAR*)msg;
	}
	else
	{
		// 4 for <> :, 1 for null
		nBytes = ((name.size() + 4 + 1) * sizeof(LIB_TCHAR)) + datBytes;
		char* msg = alloc<char>(nBytes);
		_stprintf((LIB_TCHAR*)msg, _T("<%s>: %s"), name.c_str(), dat);
		return (LIB_TCHAR*)msg;
	}
}

static LIB_TCHAR* FormatText(BYTE* dat, DWORD datBytes, UINT& nBytes, bool timeStamps)
{
	if(timeStamps)
	{
		LIB_TCHAR* time = GetTime();
		// 3 for <> :, 1 for null
		nBytes = ((_tcslen(time) + 3 + 1) * sizeof(LIB_TCHAR)) + datBytes;
		char* msg = alloc<char>(nBytes);
		_stprintf((LIB_TCHAR*)msg, _T("[%s] %s"), time, dat);
		dealloc(time);
		return (LIB_TCHAR*)msg;
	}
	else
	{
		nBytes = datBytes;
		char* msg = alloc<char>(nBytes);
		memcpy(msg, dat, nBytes);
		return (LIB_TCHAR*)msg;
	}
}


static LIB_TCHAR* FormatMsg(char type, char message, BYTE* dat, DWORD datBytes, std::tstring& name, UINT& nBytes)
{
	// 4 for <> :, 1 for null, 2 for msg
	nBytes = ((name.size() + 4 + 1) * sizeof(LIB_TCHAR)) + datBytes + 2;
	char* msg = alloc<char>(nBytes);
	msg[0] = type;
	msg[1] = message;
	_stprintf((LIB_TCHAR*)&msg[MSG_OFFSET], _T("<%s>: %s"), name.c_str(), dat);
	return (LIB_TCHAR*)msg;
}

static void PrintError(DWORD dwErr)
{
	LIB_TCHAR *szErr;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwErr, 0, (LIB_TCHAR*)&szErr, 256, NULL);
	_tprintf(_T("%s"), szErr);
	LocalFree(szErr);
}
