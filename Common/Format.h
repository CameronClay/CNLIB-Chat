#pragma once
#include "Messages.h"
#include "HeapAlloc.h"
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


static TCHAR* GetTime()
{
#if NTDDI_VERSION >= NTDDI_VISTA
	const UINT buffSize = GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, LOCALE_USE_CP_ACP, NULL, NULL, NULL, 0);
	TCHAR* buffer = alloc<TCHAR>(buffSize);
	GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, LOCALE_USE_CP_ACP, NULL, NULL, buffer, buffSize);
#else
	const UINT buffSize = GetDateFormat(LOCALE_USER_DEFAULT, LOCALE_USE_CP_ACP, NULL, NULL, NULL, 0);
	TCHAR* buffer = alloc<TCHAR>(buffSize);
	GetDateFormat(LOCALE_USER_DEFAULT, LOCALE_USE_CP_ACP, NULL, NULL, buffer, buffSize);
#endif
	return buffer;
}

static TCHAR* FormatText(BYTE* dat, DWORD datBytes, std::tstring& name, UINT& nBytes, bool timeStamps)
{
	if(timeStamps)
	{
		TCHAR* time = GetTime();
		// 7 for [] <> :, 1 for null
		nBytes = ((_tcslen(time) + name.size() + 7 + 1) * sizeof(TCHAR)) + datBytes;
		char* msg = alloc<char>(nBytes);
		_stprintf((TCHAR*)msg, _T("[%s] <%s>: %s"), time, name.c_str(), dat);
		dealloc(time);
		return (TCHAR*)msg;
	}
	else
	{
		// 4 for <> :, 1 for null
		nBytes = ((name.size() + 4 + 1) * sizeof(TCHAR)) + datBytes;
		char* msg = alloc<char>(nBytes);
		_stprintf((TCHAR*)msg, _T("<%s>: %s"), name.c_str(), dat);
		return (TCHAR*)msg;
	}
}

static TCHAR* FormatText(BYTE* dat, DWORD datBytes, UINT& nBytes, bool timeStamps)
{
	if(timeStamps)
	{
		TCHAR* time = GetTime();
		// 3 for <> :, 1 for null
		nBytes = ((_tcslen(time) + 3 + 1) * sizeof(TCHAR)) + datBytes;
		char* msg = alloc<char>(nBytes);
		_stprintf((TCHAR*)msg, _T("[%s] %s"), time, dat);
		dealloc(time);
		return (TCHAR*)msg;
	}
	else
	{
		nBytes = datBytes;
		char* msg = alloc<char>(nBytes);
		memcpy(msg, dat, nBytes);
		return (TCHAR*)msg;
	}
}


static TCHAR* FormatMsg(char type, char message, BYTE* dat, DWORD datBytes, std::tstring& name, UINT& nBytes)
{
	// 4 for <> :, 1 for null, 2 for msg
	nBytes = ((name.size() + 4 + 1) * sizeof(TCHAR)) + datBytes + 2;
	char* msg = alloc<char>(nBytes);
	msg[0] = type;
	msg[1] = message;
	_stprintf((TCHAR*)&msg[MSG_OFFSET], _T("<%s>: %s"), name.c_str(), dat);
	return (TCHAR*)msg;
}

static void PrintError(DWORD dwErr)
{
	TCHAR *szErr;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwErr, 0, (TCHAR*)&szErr, 256, NULL);
	_tprintf(_T("%s"), szErr);
	LocalFree(szErr);
}
