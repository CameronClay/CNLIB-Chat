//Copyright (c) <2015> <Cameron Clay>

#pragma once
#include "Typedefs.h"

class CAMSNETLIB File
{
public:
	File(const LIB_TCHAR* fileName, DWORD desiredAccess, DWORD fileAttributes = FILE_ATTRIBUTE_NORMAL, DWORD creationFlag = OPEN_ALWAYS);
	File();
	File(File&& file);
	~File();

	bool Open(const LIB_TCHAR* fileName, DWORD desiredAccess, DWORD fileAttributes = FILE_ATTRIBUTE_NORMAL, DWORD creationFlag = OPEN_ALWAYS);
	void Close();

	// begining or end, location FILE_BEGIN or FILE_END
	void SetCursor(UCHAR Location);
	void MoveCursor(LONG nBytes);
	DWORD GetCursor();

	DWORD Write(const void* data, DWORD nBytes);
	DWORD Read(void* dest, DWORD nBytes);

	void WriteString(const std::tstring& str);
	void WriteDate(const SYSTEMTIME& st);

	bool ReadString(std::tstring& dest);
	bool ReadDate(SYSTEMTIME& dest);

	void ChangeDate(const SYSTEMTIME& t);
	void SetAttrib(DWORD attrib);

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
		bool Valid() const;

		std::tstring fileName;
		SYSTEMTIME dateModified;
		DWORD64 size;
	};

	CAMSNETLIB void Remove(const LIB_TCHAR* fileName);
	CAMSNETLIB bool Exists(const LIB_TCHAR* fileName);

	// FILE_ATTRIBUTE_HIDDEN, FILE_ATTRIBUTE_TEMPORARY, blank = NORMAL
	CAMSNETLIB void CreateFolder(const LIB_TCHAR* fileName, DWORD fileAttributes = FILE_ATTRIBUTE_NORMAL);
	CAMSNETLIB void CreateShortcut(const LIB_TCHAR*, const LIB_TCHAR*);
	CAMSNETLIB void MoveOrRename(const LIB_TCHAR* oldFileName, const LIB_TCHAR* newFileName);
	CAMSNETLIB void GetFullFilePathName(const LIB_TCHAR* fileName, LIB_TCHAR* buffer);

	// folder = "" or full directory path, filter = NULL for no filter
	CAMSNETLIB std::vector<FileMisc::FileData> GetFileNameList(const LIB_TCHAR* folder, DWORD filter = NULL, bool inclusion = true);

	// returns true when t1 > t2, false when t2 <= t1
	CAMSNETLIB bool CompareTime(SYSTEMTIME& t1, SYSTEMTIME& t2);

	// CSIDL_DESKTOP for desktop, CSIDL_MYDOCUMENTS for documents etc.
	CAMSNETLIB HRESULT GetFolderPath(int folder, LIB_TCHAR* buffer);
	CAMSNETLIB void RemoveFolder(const LIB_TCHAR* fileName);
	CAMSNETLIB void SetCurDirectory(const LIB_TCHAR* folderName);
	CAMSNETLIB void GetCurDirectory(LIB_TCHAR* buffer);

	/*-----------CompressionFunctions------------*/
	CAMSNETLIB DWORD GetCompressedBufferSize(DWORD srcLen);
	//																					level 1-9
	CAMSNETLIB DWORD Compress(BYTE* dest, DWORD destLen, const BYTE* src, DWORD srcLen, int level);
	CAMSNETLIB DWORD Decompress(BYTE* dest, DWORD destLen, const BYTE* src, DWORD srcLen);

	// Access = FILE_GENERIC_READ, FILE_GENERIC_WRITE, FILE_GENERIC_EXECUTE, attributes = FILE_ATTRIBUTE_HIDDEN, FILE_ATTRIBUTE_TEMPORARY, blank = NORMAL
	CAMSNETLIB void SetAttrib(const LIB_TCHAR* fileName, DWORD attrib);

	static const UINT OFN_DEFAULT = OFN_CREATEPROMPT | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	/*-----------DialogFunctions------------*/
	CAMSNETLIB bool BrowseFiles(LIB_TCHAR* filePathDest, HWND hwnd, const LIB_TCHAR* fileTypes = _T("All Files\0*.*\0"), DWORD flags = OFN_DEFAULT, const LIB_TCHAR* windowName = nullptr);
	CAMSNETLIB bool BrowseFolder(const LIB_TCHAR* windowName, LIB_TCHAR* buffer, HWND hwnd, UINT flags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI);
	CAMSNETLIB bool BrowseFont(HWND hwnd, HFONT& hFont, COLORREF& color);
}

class CAMSNETLIB ProgDlg
{
public:
	ProgDlg();
	ProgDlg(ProgDlg&& progdlg);
	~ProgDlg();

	bool Start(HWND hwnd, const DWORD maxVal, const LIB_TCHAR* title, const LIB_TCHAR* line0, const LIB_TCHAR* cancelMsg);
	void Stop();

	void SetLine1(const LIB_TCHAR* line1);
	void SetProgress(DWORD progress);

	bool Canceled();
private:
	IProgressDialog* pd;
	DWORD maxVal;
	bool hasCanceled;
};
