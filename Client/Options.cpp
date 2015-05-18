#include "CNLIB\File.h"
#include "Options.h"

Options::Options(std::tstring& filePath, float currentVers)
	:
	filePath(filePath)
{
	Reset(currentVers);
}

Options::Options(Options&& opts)
	:
	filePath(opts.filePath),
	downloadPath(opts.downloadPath),
	version(opts.version),
	timeStamps(opts.timeStamps),
	startUp(opts.startUp),
	flashTaskbar(opts.flashTaskbar),
	flashCount(opts.flashCount),
	info(opts.info)
{}

Options::~Options()
{

}

void Options::Load(const LIB_TCHAR* windowName)
{
	File file(filePath.c_str(), FILE_GENERIC_READ, FILE_ATTRIBUTE_HIDDEN);

	const float prevVers = version;

	bool b = file.Read(&version, sizeof(float));
	if(version == prevVers)
	{
		b &= (
			file.ReadString(downloadPath) &&
			file.Read(&startUp, sizeof(bool)) &&
			file.Read(&flashTaskbar, sizeof(bool)) &&
			file.Read(&timeStamps, sizeof(bool)) &&
			file.Read(&flashCount, sizeof(UCHAR))
			);

		file.Close();
	}
	else
	{
		file.Close();
		file.Open(filePath.c_str(), FILE_GENERIC_READ, FILE_ATTRIBUTE_HIDDEN, CREATE_ALWAYS);
		Reset(prevVers);
		Save(windowName);
	}

}

void Options::Save(const LIB_TCHAR* windowName)
{
	File file(filePath.c_str(), FILE_GENERIC_WRITE, FILE_ATTRIBUTE_HIDDEN, CREATE_ALWAYS);


	file.Write(&version, sizeof(float));

	file.WriteString(downloadPath);

	file.Write(&startUp, sizeof(bool));
	file.Write(&flashTaskbar, sizeof(bool));
	file.Write(&timeStamps, sizeof(bool));
	file.Write(&flashCount, sizeof(UCHAR));


	file.Close();


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

void Options::SetGeneral(bool timeStamps, bool startUp, bool flashTaskbar, UINT flashCount)
{
	this->timeStamps = timeStamps;
	this->startUp = startUp;
	this->flashTaskbar = flashTaskbar;
	this->flashCount = flashCount;
}

void Options::SetDownloadPath(std::tstring& path)
{
	downloadPath = path;
}

void Options::Reset(float currentVers)
{
	filePath = filePath;
	timeStamps = true;
	startUp = false;
	flashTaskbar = true;
	flashCount = 3;
	version = currentVers;

	LIB_TCHAR buffer[MAX_PATH] = {};
	FileMisc::GetFolderPath(CSIDL_MYDOCUMENTS , buffer);
	downloadPath = std::tstring(buffer);

	// FLASHWINFO
	info.cbSize = sizeof(FLASHWINFO);
	info.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
	info.dwTimeout = 0;
}

bool Options::TimeStamps()
{
	return timeStamps;
}

bool Options::AutoStartup()
{
	return startUp;
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

UINT Options::GetFlashCount()
{
	return flashCount;
}

std::tstring& Options::GetDownloadPath()
{
	return downloadPath;
}
