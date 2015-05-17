#pragma once
#include "Typedefs.h"
#include <Shlwapi.h>
#include <Shlobj.h>
#include <stdlib.h>
#include <vector>
#include <tchar.h>

class CAMSNETLIB File
{
public:
	File(const TCHAR* fileName, DWORD desiredAccess, DWORD fileAttributes = FILE_ATTRIBUTE_NORMAL, DWORD creationFlag = OPEN_ALWAYS);
	File();
	File(File&& file);
	~File();

	bool Open(const TCHAR* fileName, DWORD desiredAccess, DWORD fileAttributes = FILE_ATTRIBUTE_NORMAL, DWORD creationFlag = OPEN_ALWAYS);
	void Close();

	// begining or end, location FILE_BEGIN or FILE_END
	void SetCursor(UCHAR Location);
	void MoveCursor(LONG nBytes);

	DWORD Write(const void* data, DWORD nBytes);
	DWORD Read(void* dest, DWORD nBytes);

	void WriteString(const std::tstring& str);
	void WriteDate(const SYSTEMTIME& st);

	bool ReadString(std::tstring& dest);
	bool ReadDate(SYSTEMTIME& dest);

	void ChangeDate(const SYSTEMTIME& t);

	bool IsOpen() const;

	DWORD64 GetSize() const;
	SYSTEMTIME GetDate() const;

private:
	HANDLE hnd;
};

namespace FileMisc
{
	struct CAMSNETLIB FileData
	{
		FileData();
		FileData(std::tstring fileName, SYSTEMTIME dateModified, DWORD64 size);
		bool operator==(const FileData& fd) const;
		bool operator<(const FileData& fd) const;
		bool operator()(FileData& fd);

		std::tstring fileName;
		SYSTEMTIME dateModified;
		DWORD64 size;
	};

	CAMSNETLIB void Remove(const TCHAR* fileName);
	CAMSNETLIB bool Exists(const TCHAR* fileName);

	// FILE_ATTRIBUTE_HIDDEN, FILE_ATTRIBUTE_TEMPORARY, blank = NORMAL
	CAMSNETLIB void CreateFolder(const TCHAR* fileName, DWORD fileAttributes = FILE_ATTRIBUTE_NORMAL);
	CAMSNETLIB void CreateShortcut(const TCHAR*, const TCHAR*);
	CAMSNETLIB void MoveOrRename(const TCHAR* oldFileName, const TCHAR* newFileName);
	CAMSNETLIB void GetFullFilePathName(const TCHAR* fileName, TCHAR* buffer);

	// folder = "" or full directory path, filter = NULL for no filter
	CAMSNETLIB void GetFileNameList(const TCHAR* folder, DWORD filter, std::vector<FileMisc::FileData>& list);

	// returns true when t1 > t2, false when t2 <= t1
	CAMSNETLIB bool CompareTime(SYSTEMTIME& t1, SYSTEMTIME& t2);

	// CSIDL_DESKTOP for desktop, CSIDL_MYDOCUMENTS for documents etc.
	CAMSNETLIB HRESULT GetFolderPath(int folder, TCHAR* buffer);
	CAMSNETLIB void RemoveFolder(const TCHAR* fileName);
	CAMSNETLIB void SetCurDirectory(const TCHAR* folderName);
	CAMSNETLIB void GetCurDirectory(TCHAR* buffer);

	/*-----------CompressionFunctions------------*/
	CAMSNETLIB DWORD GetCompressedBufferSize(DWORD srcLen);
	//																					level 1-9
	CAMSNETLIB DWORD Compress(BYTE* dest, DWORD destLen, const BYTE* src, DWORD srcLen, int level);
	CAMSNETLIB DWORD Decompress(BYTE* dest, DWORD destLen, const BYTE* src, DWORD srcLen);

	// Access = FILE_GENERIC_READ, FILE_GENERIC_WRITE, FILE_GENERIC_EXECUTE, attributes = FILE_ATTRIBUTE_HIDDEN, FILE_ATTRIBUTE_TEMPORARY, blank = NORMAL
	CAMSNETLIB void SetAttrib(const TCHAR* fileName, DWORD attrib);

	/*-----------DialogFunctions------------*/
	CAMSNETLIB bool BrowseFiles(const TCHAR* windowName, TCHAR* filePathDest, HWND hwnd, DWORD flags = OFN_CREATEPROMPT | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST);
	CAMSNETLIB bool BrowseFolder(const TCHAR* windowName, TCHAR* buffer, HWND hwnd, UINT flags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI);
	CAMSNETLIB bool BrowseFont(HWND hwnd, HFONT& hFont, COLORREF& color);
}

class CAMSNETLIB ProgDlg
{
public:
	ProgDlg();
	ProgDlg(ProgDlg&& progdlg);
	~ProgDlg();

	bool Start(HWND hwnd, const DWORD maxVal, const TCHAR* title, const TCHAR* line0, const TCHAR* cancelMsg);
	void Stop();

	void SetLine1(const TCHAR* line1);
	void SetProgress(DWORD progress);

	bool Canceled();
private:
	IProgressDialog* pd;
	DWORD maxVal;
	bool hasCanceled;
};
