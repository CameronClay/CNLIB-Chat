#pragma once
#include <Windows.h>
#include "Format.h"

class Options
{
public:
	Options(std::tstring& filePath, float currentVers);
	Options(Options&& opts);
	~Options();

	void Load(const LIB_TCHAR* windowName);
	void Save(const LIB_TCHAR* windowName);

	void SetGeneral(bool timeStamps, bool startUp, bool flashTaskbar, UINT flashCount);
	void SetDownloadPath(std::tstring& path);

	void Reset(float currentVers);

	bool TimeStamps();
	bool AutoStartup();
	bool FlashTaskbar(HWND hWnd);
	UINT GetFlashCount();
	std::tstring& GetDownloadPath();
private:
	std::tstring filePath;
	FLASHWINFO info;
	std::tstring downloadPath;
	float version;
	bool timeStamps, startUp, flashTaskbar;
	UCHAR flashCount;
};