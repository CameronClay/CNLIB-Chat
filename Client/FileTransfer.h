#pragma once
#include "CNLIB\TCPClientInterface.h"
#include "CNLIB\File.h"

typedef void(*AlertFunc)(std::tstring& name);

class FileTransfer
{
public:
	FileTransfer(TCPClientInterface& client, HWND wnd, AlertFunc finished, AlertFunc canceled, std::vector<FileMisc::FileData>&& list);
	FileTransfer(TCPClientInterface& client, HWND wnd, AlertFunc finished, AlertFunc canceled);
	FileTransfer(FileTransfer&& ft);

	~FileTransfer();

	void SetSize(double size);
	double GetSize() const;

	ProgDlg& GetDialog();
	HWND GetWnd() const;
	std::vector<FileMisc::FileData>& GetList();
	std::vector<FileMisc::FileData>::iterator& GetIterator();

	std::tstring& GetUser();
	bool Running() const;

	void RunFinished();
	void RunCanceled();

	void Stop();
protected:
	void SetList(std::vector<FileMisc::FileData>&& list);

	TCPClientInterface& client;
	HWND wnd;

	std::vector<FileMisc::FileData> list;
	std::vector<FileMisc::FileData>::iterator it;
	double size, progress;

	ProgDlg dialog;
	bool running, canceled;

	std::tstring username;

	AlertFunc finishedFunc, canceledFunc;
};

class FileSend : public FileTransfer
{
public:
	FileSend(TCPClientInterface& client, HWND wnd, AlertFunc finished, AlertFunc canceled, std::vector<FileMisc::FileData>&& list, DWORD nBytesPerLoop = 100000);
	FileSend(TCPClientInterface& client, HWND wnd, AlertFunc finished, AlertFunc canceled, DWORD nBytesPerLoop = 100000);
	FileSend(FileSend&& ft);
	~FileSend();

	void SetFullPathSrc(std::tstring& fullFilepathSrc);
	void RequestTransfer();
	void SendFileNameList();
	void SendCurrentFile();

	void StartSend();
	void StopSend();

	void WaitForThread();

	std::tstring& GetFilePathSrc();
	HANDLE& GetThread();
private:
	std::tstring fullFilepathSrc;
	DWORD nBytesPerLoop;
	HANDLE thread;
	DWORD threadID;
};

class MsgStreamReader;

class FileReceive : public FileTransfer
{
public:
	FileReceive(TCPClientInterface& client, HWND wnd, AlertFunc finished, AlertFunc canceled);
	FileReceive(FileReceive&& ft);
	~FileReceive();

	void RecvFileNameList(MsgStreamReader& streamReader, std::tstring& downloadPath);
	void RecvFile(BYTE* data, DWORD nBytes);

	void StopReceive();
private:
	void CloseAndReplaceFile();

	File file;
	DWORD64 bytesWritten;
	LIB_TCHAR tempFilename[MAX_PATH + 75] = {};
};