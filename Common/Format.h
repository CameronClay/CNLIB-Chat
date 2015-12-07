#pragma once
#include "CNLIB\Messages.h"
#include "CNLIB\HeapAlloc.h"
#include "CNLIB\Typedefs.h"

struct Format
{
	static UINT GetTimeCharCount()
	{
		return GetTimeFormat(LOCALE_USER_DEFAULT, LOCALE_USE_CP_ACP, NULL, NULL, NULL, 0);
	}
	static UINT GetTime(LIB_TCHAR* buffer, UINT charCount)
	{
		return GetTimeFormat(LOCALE_USER_DEFAULT, LOCALE_USE_CP_ACP, NULL, NULL, buffer, charCount);
	}

	static UINT FormatTextBuffLen(const LIB_TCHAR* dat, DWORD strLen, bool timeStamps)
	{
		//[3:14:16 PM] <Luis>: 
		// 3 for "[] ", 1 for null(Time char count includes)
		// 1 for null
		return timeStamps ? ((GetTimeCharCount() + strLen + 3)) : strLen;
	}

	static UINT FormatTextBuffLen(const LIB_TCHAR* dat, DWORD strLen, const std::tstring& name, bool timeStamps)
	{
		//4 for "<>: "
		return FormatTextBuffLen(dat, strLen, timeStamps) + name.size() + 4;
	}

	static void FormatText(const LIB_TCHAR* data, LIB_TCHAR* buffer, UINT buffLen, const std::tstring& name, bool timeStamps)
	{
		if (timeStamps)
		{
			const UINT timeLen = GetTimeCharCount();
			LIB_TCHAR* time = alloc<LIB_TCHAR>(timeLen);
			GetTime(time, timeLen);

			_stprintf(buffer, _T("[%s] <%s>: %s"), time, name.c_str(), data);
			dealloc(time);
		}
		else
		{
			_stprintf(buffer, _T("<%s>: %s"), name.c_str(), data);
		}
	}
	static void FormatText(const LIB_TCHAR* data, LIB_TCHAR* buffer, UINT buffLen, bool timeStamps)
	{
		if (timeStamps)
		{
			const UINT timeLen = GetTimeCharCount();
			LIB_TCHAR* time = alloc<LIB_TCHAR>(timeLen);
			GetTime(time, timeLen);

			_stprintf(buffer, _T("[%s] %s"), time, data);
			dealloc(time);
		}
		else
		{
			memcpy(buffer, data, buffLen);
		}
	}
};
