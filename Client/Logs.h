#pragma once
#include "Format.h"
#include "CNLIB\File.h"
#include <stdlib.h>
#include <vector>

class Logs
{
public:
	Logs()
		:
		logList()
	{}
	Logs(Logs&& logs)
		:
		logList(std::move(logs.logList))
	{
		ZeroMemory(&logs, sizeof(Logs));
	}
	~Logs(){}

	void Load(const TCHAR* path)
	{
		FileMisc::GetFileNameList(path, NULL, logList);
	}

	void AddLog(const char* data, DWORD nBytes)
	{
		TCHAR buffer[128] = _T("Log ");
		TCHAR temp[5];
		const size_t index = logList.size() + 1;
		_tcscat(buffer, _itot(index, temp, 10));

		File log(buffer, FILE_GENERIC_WRITE, FILE_ATTRIBUTE_NORMAL, CREATE_ALWAYS);
		log.Write(data, nBytes);

		SYSTEMTIME currentTime;
		GetSystemTime(&currentTime);
		logList.push_back(FileMisc::FileData(std::tstring(buffer), currentTime, nBytes));
	}

	//if nBytes is specified copies that many bytes into dest, otherwise outputs the number of bytes into the nBytes var
	void ReadLog(const TCHAR* filename, char* dest = nullptr, DWORD64* nBytes = nullptr)
	{
		File log(filename, FILE_GENERIC_READ);
		if(nBytes)
			log.Read(dest, *nBytes);
		else
			*nBytes = log.GetSize();
	}

	std::vector<FileMisc::FileData>& GetLogList()
	{
		return logList;
	}
private:
	std::vector<FileMisc::FileData> logList;
};