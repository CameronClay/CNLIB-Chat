#pragma once
//#include "CNLIB/Typedefs.h"

class TextDisplay
{
public:
	TextDisplay()
		:
		hnd(NULL),
		text()
	{}

	void SetHandle(HWND hnd)
	{
		this->hnd = hnd;
	}
	void WriteLine(const std::tstring& str)
	{
		if (!text.empty())
			text.append(_T("\r\n"));

		text.append(str);

		SendMessage(hnd, WM_SETTEXT, 0, (LPARAM)text.c_str());
		SendMessage(hnd, EM_LINESCROLL, 0, MAXLONG);
	}
	void WriteLine(LIB_TCHAR* buffer, DWORD length)
	{
		if (!text.empty())
			text.append(_T("\r\n"));

		text.append(buffer, length);

		SendMessage(hnd, WM_SETTEXT, 0, (LPARAM)text.c_str());
		SendMessage(hnd, EM_LINESCROLL, 0, MAXLONG);
	}
	const std::tstring& GetText() const
	{
		return text;
	}
private:
	HWND hnd;
	std::tstring text;
};
