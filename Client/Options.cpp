#include "StdAfx.h"
#include "Options.h"
//#include "CNLIB\File.h"

Options::Options(std::tstring& filePath, float currentVers)
{
	Reset(filePath, currentVers);
}

Options::Options(Options&& opts)
	:
	Logs(std::forward<Logs>(opts)),
	filePath(opts.filePath),
	downloadPath(opts.downloadPath),
	version(opts.version),
	timeStamps(opts.timeStamps),
	startUp(opts.startUp),
	flashTaskbar(opts.flashTaskbar),
	saveLogs(opts.saveLogs),
	flashCount(opts.flashCount),
	info(opts.info)
{}

Options::~Options()
{}

void Options::Load(const LIB_TCHAR* windowName)
{
	File file(filePath.c_str(), FILE_GENERIC_READ, FILE_ATTRIBUTE_HIDDEN);

	const float prevVers = version;

	bool b = file.Read(&version, sizeof(float)) == sizeof(float);
	if(version == prevVers)
	{
		b &= (
			file.ReadString(downloadPath) &&
			file.Read(&startUp, sizeof(bool)) &&
			file.Read(&flashTaskbar, sizeof(bool)) &&
			file.Read(&timeStamps, sizeof(bool)) &&
			file.Read(&saveLogs, sizeof(bool)) &&
			file.Read(&flashCount, sizeof(UCHAR))
			);
	}
	else
	{
		file.Open(filePath.c_str(), FILE_GENERIC_READ, FILE_ATTRIBUTE_HIDDEN, CREATE_ALWAYS);
		Reset(filePath, prevVers);
		Save(windowName);
	}


	const size_t pathSize = filePath.size();
	LIB_TCHAR* buffer = alloc<LIB_TCHAR>(pathSize + 64);
	_tcscpy(buffer, filePath.c_str());

	PathRemoveFileSpec(buffer);

	_tcscat(buffer, _T("\\Logs"));

	if(!FileMisc::Exists(buffer))
		FileMisc::CreateFolder(buffer);

	LoadLogList(buffer);

	if (SaveLogs())
		CreateLog();

	dealloc(buffer);
}

void Options::Save(const LIB_TCHAR* windowName)
{
	File file(filePath.c_str(), FILE_GENERIC_WRITE, FILE_ATTRIBUTE_HIDDEN, CREATE_ALWAYS);


	file.Write(&version, sizeof(float));

	file.WriteString(downloadPath);

	file.Write(&startUp, sizeof(bool));
	file.Write(&flashTaskbar, sizeof(bool));
	file.Write(&timeStamps, sizeof(bool));
	file.Write(&saveLogs, sizeof(bool));
	file.Write(&flashCount, sizeof(UCHAR));

	if(startUp)
	{
		LIB_TCHAR path[MAX_PATH];
		GetModuleFileName(NULL, path, MAX_PATH);

#if NTDDI_VERSION >= NTDDI_VISTA
		RegSetKeyValue(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), windowName, REG_SZ, path, (lstrlen(path) + 1) * sizeof(LIB_TCHAR));
#else
		HKEY key;
		RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, NULL);
		RegSetValueEx(key, windowName, NULL, REG_SZ, (const BYTE*)path, (lstrlen(path) + 1) * sizeof(LIB_TCHAR));
		RegCloseKey(key);
#endif
	}

	else
	{
#if NTDDI_VERSION >= NTDDI_VISTA
		RegDeleteKeyValue(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), windowName);
#else
		HKEY key;
		RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), NULL, KEY_WRITE, &key);
		RegDeleteValue(key, windowName);
		RegCloseKey(key);
#endif
	}
}

void Options::SetGeneral(bool timeStamps, bool startUp, bool flashTaskbar, bool saveLogs, UCHAR flashCount)
{
	this->timeStamps = timeStamps;
	this->startUp = startUp;
	this->flashTaskbar = flashTaskbar;
	this->saveLogs = saveLogs;
	this->flashCount = flashCount;
}

void Options::SetDownloadPath(const std::tstring& path)
{
	downloadPath = path;
}

void Options::Reset(std::tstring& filePath, float currentVers)
{
	this->filePath = filePath;
	timeStamps = true;
	startUp = false;
	flashTaskbar = true;
	saveLogs = true;
	flashCount = 3;
	version = currentVers;

	LIB_TCHAR buffer[MAX_PATH] = {};
	FileMisc::GetFolderPath(CSIDL_MYDOCUMENTS , buffer);
	downloadPath = buffer;

	// FLASHWINFO
	info.cbSize = sizeof(FLASHWINFO);
	info.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
	info.dwTimeout = 0;
}

bool Options::TimeStamps() const
{
	return timeStamps;
}

bool Options::AutoStartup() const
{
	return startUp;
}

bool Options::SaveLogs() const
{
	return saveLogs;
}

bool Options::FlashTaskbar(HWND hWnd)
{
	if (flashTaskbar)
	{
		info.hwnd = hWnd;
		info.uCount = flashCount;

		FlashWindowEx(&info);
	}

	return flashTaskbar;
}

UINT Options::GetFlashCount() const
{
	return flashCount;
}

const std::tstring& Options::GetDownloadPath() const
{
	return downloadPath;
}
