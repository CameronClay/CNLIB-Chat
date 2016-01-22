#pragma once
#include <Windows.h>
#include "Format.h"
#include "Logs.h"

class Options : public Logs
{
public:
	Options(std::tstring& filePath, float currentVers);
	Options(Options&& opts);
	~Options();

	void Load(const LIB_TCHAR* windowName);
	void Save(const LIB_TCHAR* windowName);

	void SetGeneral(bool timeStamps, bool startUp, bool flashTaskbar, bool saveLogs, UCHAR flashCount);
	void SetDownloadPath(const std::tstring& path);

	void Reset(std::tstring& filePath, float currentVers);

	bool TimeStamps() const;
	bool AutoStartup() const;
	bool SaveLogs() const;
	UINT GetFlashCount() const;
	bool FlashTaskbar(HWND hWnd);
	const std::tstring& GetDownloadPath() const;
private:
	std::tstring filePath;
	FLASHWINFO info;
	std::tstring downloadPath;
	float version;
	bool timeStamps, startUp, flashTaskbar, saveLogs;
	UCHAR flashCount;
};
