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

	static void FormatText(const std::tstring& str, std::tstring& dest, const std::tstring& name, bool timeStamps)
	{
		if (timeStamps)
		{
			const UINT timeLen = GetTimeCharCount();
			LIB_TCHAR* time = alloc<LIB_TCHAR>(timeLen);
			GetTime(time, timeLen);

			std::tstringstream stringStream;
			stringStream << _T("[") << time << _T("]") << _T(" <") << name << _T(">: ") << str;
			dest = stringStream.str();
			//_stprintf(buffer, _T("[%s] <%s>: %s"), time, name.c_str(), data);
			dealloc(time);
		}
		else
		{
			std::tstringstream stringStream;
			stringStream <<  _T("<") << name << _T(">: ") << str;
			dest = stringStream.str();
			//_stprintf(buffer, _T("<%s>: %s"), name.c_str(), data);
		}
	}
	static void FormatText(const std::tstring& str, std::tstring& dest, bool timeStamps)
	{
		if (timeStamps)
		{
			const UINT timeLen = GetTimeCharCount();
			LIB_TCHAR* time = alloc<LIB_TCHAR>(timeLen);
			GetTime(time, timeLen);

			std::tstringstream stringStream;
			stringStream << _T("[") << time << _T("] ") << str;
			dest = stringStream.str();
			//_stprintf(buffer, _T("[%s] %s"), time, data);
			dealloc(time);
		}
		else
		{
			dest = str;
		}
	}
};
