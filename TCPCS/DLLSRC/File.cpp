#include "File.h"

#include <assert.h>
#include <Shlwapi.h>
#include <Shlobj.h>
#include <sstream>
#include "HeapAlloc.h"

#include "zlib/include/zlib.h"

#pragma comment( lib, "shlwapi.lib" )
#pragma comment( lib, "zlib/lib/zdll.lib" )




FileMisc::FileData::FileData()
{}

FileMisc::FileData::FileData(std::tstring fileName, SYSTEMTIME dateModified, DWORD64 size)
	: fileName(fileName), dateModified(dateModified), size(size)
{}

bool FileMisc::FileData::operator==(const FileData& fd) const
{
	return fileName.compare(fd.fileName) == 0;
}

bool FileMisc::FileData::operator<(const FileData& fd) const
{
	return fileName.compare(fd.fileName) < 0;
}

bool FileMisc::FileData::operator()(FileData& fd)
{
	if(fileName.compare(fd.fileName) == 0)
		return !CompareTime(fd.dateModified, dateModified);
	return false;
}

File::File(const TCHAR* fileName, DWORD desiredAccess, DWORD fileAttributes, DWORD creationFlag)
{
	bool b = Open(fileName, desiredAccess, fileAttributes, creationFlag);
	assert(b);
}

File::File()
	:
	hnd(NULL)
{}

File::File(File&& file)
	:
	hnd(file.hnd)
{
	ZeroMemory(&file, sizeof(File));
}

File::~File()
{
	Close();
}

bool File::Open(const TCHAR* fileName, DWORD desiredAccess, DWORD fileAttributes, DWORD creationFlag)
{
	return (hnd = CreateFile(fileName, desiredAccess, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, creationFlag, fileAttributes, NULL)) != NULL;
}

void File::Close()
{
	if(hnd)
	{
		CloseHandle(hnd);
		hnd = NULL;
	}
}

void File::SetCursor(UCHAR Location)
{
	SetFilePointer(hnd, 0, NULL, Location);
}

void File::MoveCursor(LONG nBytes)
{
	SetFilePointer(hnd, nBytes, NULL, FILE_CURRENT);
}

DWORD File::Write(const void* data, DWORD nBytes)
{
	DWORD nBytesWritten = 0;
	WriteFile(hnd, data, nBytes, &nBytesWritten, NULL);
	return nBytesWritten;
}

DWORD File::Read(void* dest, DWORD nBytes)
{
	DWORD nBytesRead = 0;
	ReadFile(hnd, dest, nBytes, &nBytesRead, NULL);
	return nBytesRead;
}

void File::WriteString(std::tstring& str)
{
	std::tstring t(To_String(str.length()) + _T(":") + str);
	Write(t.c_str(), sizeof(TCHAR) * t.length());
}

void File::WriteDate(SYSTEMTIME& st)
{
	WCHAR dstr[25], tstr[25];

#if NTDDI_VERSION >= NTDDI_VISTA
	GetDateFormatEx(LOCALE_NAME_SYSTEM_DEFAULT, LOCALE_USE_CP_ACP, &st, NULL, dstr, 25, NULL);
	GetTimeFormatEx(LOCALE_NAME_SYSTEM_DEFAULT, LOCALE_USE_CP_ACP | TIME_FORCE24HOURFORMAT, &st, NULL, tstr, 25);
#else
	GetDateFormat(LOCALE_USER_DEFAULT, LOCALE_USE_CP_ACP, &st, NULL, dstr, 25);
	GetTimeFormat(LOCALE_USER_DEFAULT, LOCALE_USE_CP_ACP | TIME_FORCE24HOURFORMAT, &st, NULL, tstr, 25);
#endif

	std::tstring str;
#ifdef UNICODE
	str = dstr + std::tstring(_T(" ")) + tstr;
#else
	TCHAR date[25], time[25];
	WideCharToMultiByte(CP_ACP, 0, dstr, -1, date, 25, NULL, NULL);
	WideCharToMultiByte(CP_ACP, 0, tstr, -1, time, 25, NULL, NULL);
	str = date + std::tstring(_T(" ")) + time;
#endif
	Write(str.c_str(), sizeof(TCHAR) * str.length());
}

bool File::ReadString(std::tstring& dest)
{
	UINT i = 0;
	std::tstring len = _T("0");
	do
	{
		Read(&len[i], sizeof(TCHAR));
		len.append(_T(":"));
	} while(len[i++] != ':');

	UINT strLen = std::stoul(len);
	if(strLen)
	{
		TCHAR *str = alloc<TCHAR>(strLen + 1);
		Read(str, sizeof(TCHAR) * strLen);
		str[strLen] = '\0', dest = str;
		dealloc(str);
	}
	return strLen != 0;
}

bool File::ReadDate(SYSTEMTIME& dest)
{
	bool b;
	TCHAR c;
	std::stringstream buf;
	do
	{
		b = Read(&c, sizeof(TCHAR)) != 0;
		buf << (c == '/' || c == ':' ? ' ' : c);
	} while(c != '\n');
	buf << '\0';

	memset(&dest, 0, sizeof(SYSTEMTIME));
	buf >> dest.wMonth;
	buf >> dest.wDay;
	buf >> dest.wYear;
	buf >> dest.wHour;
	buf >> dest.wMinute;
	buf >> dest.wSecond;

	FILETIME ft;
	SystemTimeToFileTime(&dest, &ft);
	FileTimeToSystemTime(&ft, &dest);
	return b;
}


bool File::IsOpen() const
{
	return (hnd != nullptr);
}

DWORD64 File::GetSize() const
{
	DWORD szHigh;
	DWORD szLow = GetFileSize(hnd, &szHigh);
	return DWORD64(szHigh) << 32 | szLow;
}

SYSTEMTIME File::GetDate() const
{
	FILETIME fTime;
	GetFileTime(hnd, NULL, NULL, &fTime);
	SYSTEMTIME sTime;
	FileTimeToSystemTime(&fTime, &sTime);
	return sTime;
}

void File::ChangeDate(const SYSTEMTIME& t)
{
	FILETIME ft;
	SystemTimeToFileTime(&t, &ft);
	SetFileTime(hnd, nullptr, nullptr, &ft);
}

void FileMisc::SetAttrib(const TCHAR* fileName, DWORD attrib)
{
	SetFileAttributes(fileName, attrib);
}

void FileMisc::CreateFolder(const TCHAR* fileName, DWORD fileAttributes)
{
	if(!CreateDirectory(fileName, NULL))
	{
		if(GetLastError() == ERROR_PATH_NOT_FOUND)
		{
			TCHAR *buff = _tcsdup(fileName);
			PathRemoveFileSpec(buff);
			CreateFolder(buff);
			CreateDirectory(fileName, NULL);
			free(buff);
		}
	}
	SetFileAttributes(fileName, fileAttributes);
}

void FileMisc::CreateShortcut(const TCHAR* target, const TCHAR* linkName)
{
	IShellLink *sl; IPersistFile *pf;
	//CLSID_FolderShortcut
	HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void**)&sl);
	if(SUCCEEDED(hr))
	{
		TCHAR buff[MAX_PATH];
		if(PathIsRelative(target))
		{
			GetFullFilePathName(target, buff);
			sl->SetPath(buff);
		}
		else
			sl->SetPath(target);
		_tcscpy(buff, target);
		PathRemoveFileSpec(buff);
		sl->SetWorkingDirectory(buff);
		hr = sl->QueryInterface(IID_IPersistFile, (void**)&pf);
		if(SUCCEEDED(hr))
		{
			WCHAR link[MAX_PATH];
#ifndef UNICODE
			MultiByteToWideChar(CP_ACP, 0, buff, MAX_PATH, link, MAX_PATH);
#endif
			pf->Save(link, true);
			pf->Release();
		}
		sl->Release();
	}
}

void FileMisc::MoveOrRename(const TCHAR* oldFileName, const TCHAR* newFileName)
{
	MoveFile(oldFileName, newFileName);
}

void FileMisc::Remove(const TCHAR* fileName)
{
	DeleteFile(fileName);
}

void FileMisc::RemoveFolder(const TCHAR* fileName)
{
	RemoveDirectory(fileName);
}

bool FileMisc::Exists(const TCHAR* fileName)
{
	return PathFileExists(fileName) != FALSE;
}

void FileMisc::SetCurDirectory(const TCHAR* folderName)
{
	SetCurrentDirectory(folderName);
}

void FileMisc::GetCurDirectory(TCHAR* buffer)
{
	GetCurrentDirectory(MAX_PATH, buffer);
}

HRESULT FileMisc::GetFolderPath(int folder, TCHAR* buffer)
{
	return SHGetFolderPath(NULL, folder, NULL, SHGFP_TYPE_CURRENT, buffer);
}

void FileMisc::GetFullFilePathName(const TCHAR* fileName, TCHAR* buffer)
{
	GetFullPathName(fileName, MAX_PATH, buffer, NULL);
}



DWORD FileMisc::GetCompressedBufferSize(DWORD srcLen)
{
	return compressBound(srcLen);
}

DWORD FileMisc::Compress(BYTE* dest, DWORD destLen, const BYTE* src, DWORD srcLen, int level)
{
	DWORD nBytes = destLen;
	const int result = compress2(dest, &nBytes, src, srcLen, level);
	assert(result != Z_MEM_ERROR);
	assert(result != Z_BUF_ERROR);
	assert(result != Z_STREAM_ERROR);
	return nBytes;
}

DWORD FileMisc::Decompress(BYTE* dest, DWORD destLen, const BYTE* src, DWORD srcLen)
{
	DWORD nBytes = destLen;
	const int result = uncompress(dest, &nBytes, src, srcLen);
	assert(result != Z_MEM_ERROR);
	assert(result != Z_BUF_ERROR);
	assert(result != Z_DATA_ERROR);
	return nBytes;
}


void FileMisc::GetFileNameList(const TCHAR* folder, DWORD filter, std::vector<FileMisc::FileData>& list)
{
	WIN32_FIND_DATA fileSearch;
	TCHAR buffer[MAX_PATH] = {};
	_stprintf_s(buffer, _T("%s\\*"), folder);


	HANDLE hnd = FindFirstFile(buffer, &fileSearch);
	if(hnd != INVALID_HANDLE_VALUE)
	{
		do
		{
			if(!(fileSearch.dwFileAttributes & filter))
			{
				if(fileSearch.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					if(_tcscmp(fileSearch.cFileName, _T(".")) != 0 && _tcscmp(fileSearch.cFileName, _T("..")) != 0)
					{
						//std::tstring temp = folder != "" ? std::tstring( folder ) + std::tstring("\\") : "";

						std::vector<FileData> a;
						GetFileNameList((std::tstring(folder) + _T("\\") + std::tstring(fileSearch.cFileName)).c_str(), filter, a);
						for(auto i = a.begin(); i != a.end(); i++)
						{
							list.push_back(FileData(std::tstring(fileSearch.cFileName) + _T("\\") + i->fileName, i->dateModified, i->size));
						}
					}
				}
				else
				{
					SYSTEMTIME t;
					FileTimeToSystemTime(&fileSearch.ftLastWriteTime, &t);
					list.push_back(FileData(fileSearch.cFileName, t, DWORD64(fileSearch.nFileSizeHigh) << 32 | fileSearch.nFileSizeLow));
				}
			}
		} while(FindNextFile(hnd, &fileSearch));
		FindClose(hnd);
	}
}

bool FileMisc::CompareTime(SYSTEMTIME& t1, SYSTEMTIME& t2)
{
	bool res = false;
	if(t1.wYear > t2.wYear)
		res = true;
	else if(t1.wYear == t2.wYear)
	{
		if(t1.wMonth > t2.wMonth)
			res = true;
		else if(t1.wMonth == t2.wMonth)
		{
			if(t1.wDay > t2.wDay)
				res = true;
			else if(t1.wDay == t2.wDay)
			{
				if(t1.wHour > t2.wHour)
					res = true;
				else if(t1.wHour == t2.wHour)
				{
					if(t1.wMinute > t2.wMinute)
						res = true;
					else if(t1.wMinute == t2.wMinute)
					{
						if(t1.wSecond > t2.wSecond)
							res = true;
					}
				}
			}
		}
	}
	return res;
}


//HKEY File::CreateRegistryKey(HKEY hnd, const TCHAR* path, DWORD accessRights)
//{
//	HKEY key;
//	RegCreateKeyEx(hnd, path, 0, NULL, 0, accessRights, NULL, &key, NULL);
//	return key;
//}
//
//bool File::SetRegistryKeyValue(HKEY hnd, const TCHAR* subKey, const TCHAR* valueName, DWORD type, const BYTE* data, DWORD nBytes)
//{
//	return RegSetKeyValue(hnd, subKey, valueName, type, data, nBytes) == ERROR_SUCCESS ? true : false;
//}
//
//HKEY File::OpenRegistryKey(HKEY hnd, const TCHAR* subKey, DWORD accessRights)
//{
//	HKEY key;
//	RegOpenKeyEx(hnd, subKey, 0, accessRights, &key);
//	return key;
//}
//
//bool File::KeyExists(HKEY hnd, const TCHAR* subKey)
//{
//	HKEY key = File::OpenRegistryKey(HKEY_CURRENT_USER, subKey);
//	if (key == ERROR_SUCCESS)
//	{
//		File::CloseRegistryKey(key);
//		return true;
//	}
//	return false;
//}
//
//bool File::SaveKey(HKEY hnd, const TCHAR* name)
//{
//	bool b = (RegSaveKey(hnd, name, NULL) == ERROR_SUCCESS ? true : false);
//	return b;
//}
//
//void File::CloseRegistryKey(HKEY hnd)
//{
//	RegCloseKey(hnd);
//}
//
//bool File::DeleteRegistryKeyValue(HKEY hnd, const TCHAR* subKey, const TCHAR* valueName)
//{
//	return RegDeleteKeyValue(hnd, subKey, valueName) == ERROR_SUCCESS ? true : false;
//}


bool FileMisc::BrowseFiles(const TCHAR* windowName, TCHAR* filePathDest, HWND hwnd, DWORD flags)
{
	OPENFILENAME file;

	ZeroMemory(&file, sizeof(file));
	file.lStructSize = sizeof(file);
	file.hwndOwner = hwnd;
	file.lpstrDefExt = _T("txt");
	file.lpstrFile = filePathDest;
	file.lpstrFile[0] = '\0';
	file.nMaxFile = MAX_PATH;
	file.lpstrTitle = windowName;
	file.Flags = flags;

	return GetOpenFileName(&file) != 0;
}

bool FileMisc::BrowseFolder(const TCHAR* windowName, TCHAR* buffer, HWND hwnd, UINT flags)
{
	BROWSEINFO bi = { 0 };
	bi.hwndOwner = hwnd;
	bi.lpszTitle = windowName;
	bi.ulFlags = flags;

	LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
	bool itemListValid = (pidl != nullptr);

	if (itemListValid)
	{
		SHGetPathFromIDList(pidl, buffer);
		CoTaskMemFree(pidl);
	}

	return itemListValid;
}

bool FileMisc::BrowseFont(HWND hwnd, HFONT& hFont, COLORREF& color)
{
	LOGFONT log;
	CHOOSEFONT font = { sizeof(CHOOSEFONT), hwnd, NULL, &log, 0, CF_SCREENFONTS | CF_EFFECTS, RGB(0, 0, 0) };
	if(ChooseFont(&font))
	{
		color = font.rgbColors;
		hFont = CreateFontIndirect(&log);
		return true;
	}
	return false;
}

ProgDlg::ProgDlg()
	:
	pd(nullptr),
	hasCanceled(false)
{
	CoInitialize(NULL);
}

ProgDlg::ProgDlg(ProgDlg&& progdlg)
	:
	pd(progdlg.pd),
	maxVal(progdlg.maxVal),
	hasCanceled(progdlg.hasCanceled)
{
	ZeroMemory(&progdlg, sizeof(ProgDlg));
}


ProgDlg::~ProgDlg()
{
	Stop();
	CoUninitialize();
}

bool ProgDlg::Start(HWND hwnd, const DWORD maxVal, const TCHAR* title, const TCHAR* line0, const TCHAR* cancelMsg)
{
	if(!SUCCEEDED(CoCreateInstance(CLSID_ProgressDialog, NULL, CLSCTX_INPROC_SERVER, IID_IProgressDialog, (void**)&pd)) || pd == nullptr)
		return false;

	pd->StartProgressDialog(hwnd, NULL, PROGDLG_AUTOTIME, NULL);

#ifdef UNICODE
	pd->SetTitle(title);
	//          1 due to PROGDLG_AUTOTIME
	pd->SetLine(1, line0, false, NULL);
	pd->SetCancelMsg(cancelMsg, NULL);
#else
	WCHAR buff[128];
	MultiByteToWideChar(CP_ACP, 0, title, 128, buff , 128);
	pd->SetTitle(buff);
	MultiByteToWideChar(CP_ACP, 0, line0, 128, buff, 128);
	pd->SetLine(0, buff, false, NULL);
	MultiByteToWideChar(CP_ACP, 0, cancelMsg, 128, buff, 128);
	pd->SetCancelMsg(buff, NULL);
#endif

	this->maxVal = maxVal;
	pd->SetProgress(0, maxVal);

	return true;
}

void ProgDlg::Stop()
{
	if(pd)
	{
		pd->StopProgressDialog();
		pd->Release();
		pd = nullptr;

		maxVal = 0;
		hasCanceled = false;
	}
}

void ProgDlg::SetLine1(const TCHAR* line1)
{
#ifdef UNICODE
	//          2 due to PROGDLG_AUTOTIME
	pd->SetLine(2, line1, true, NULL);
#else
	WCHAR buff[128];
	MultiByteToWideChar(CP_ACP, 0, line1, 128, buff, 128);
	pd->SetLine(1, buff, true, NULL);
#endif
}
void ProgDlg::SetProgress(DWORD progress)
{
	pd->SetProgress(progress, maxVal);
}

bool ProgDlg::Canceled()
{
	if(!pd) return true;

	else if(pd->HasUserCancelled() && !hasCanceled)
	{
		hasCanceled = true;
		return true;
	}

	return false;
}

