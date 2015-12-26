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

	void LoadLogList(const std::tstring& path)
	{
		this->path = path;
		FileMisc::GetFileNameList(path.c_str(), NULL, logList);
	}

	void AddLog(const char* data, DWORD nBytes)
	{
		TCHAR buffer[512];
		_stprintf(buffer, _T("%s\\Log%.2d.txt"), path.c_str(), logList.size());

		File log(buffer, FILE_GENERIC_WRITE, FILE_ATTRIBUTE_NORMAL, CREATE_ALWAYS);

		const DWORD maxCompLen = FileMisc::GetCompressedBufferSize(nBytes);
		char* compBuffer = alloc<char>(maxCompLen);
		const DWORD compLen = FileMisc::Compress((BYTE*)compBuffer, maxCompLen, (const BYTE*)data, nBytes, 9);
		log.Write((void*)&nBytes, sizeof(DWORD));
		log.Write((void*)&compLen, sizeof(DWORD));
		log.Write(compBuffer, compLen);

		dealloc(compBuffer);

		SYSTEMTIME currentTime;
		GetSystemTime(&currentTime);
		logList.push_back(FileMisc::FileData(std::tstring(buffer), currentTime, nBytes));
	}

	//if nBytes is specified copies that many bytes into dest, otherwise outputs the number of bytes into the nBytes var
	void ReadLog(int index, char* dest = nullptr, DWORD* nBytes = nullptr)
	{
		File log((path + _T("\\") + logList[index].fileName).c_str(), FILE_GENERIC_READ);

		if(dest)
		{
			DWORD compLen;
			log.MoveCursor(sizeof(DWORD));
			log.Read((void*)&compLen, sizeof(DWORD));

			char* compBuffer = alloc<char>(compLen);
			log.Read(compBuffer, compLen);
			FileMisc::Decompress((BYTE*)dest, *nBytes, (const BYTE*)compBuffer, compLen);
			dealloc(compBuffer);
		}
		else
		{
			DWORD read = log.Read((void*)nBytes, sizeof(DWORD));
		}
	}

	void RemoveLog(int index)
	{
		FileMisc::Remove((path + _T("\\") + logList[index].fileName).c_str());

		//change all log's index above the removed log -1
		for(int i = index + 1, size = logList.size(); i < size; i++)
		{
			std::tstring curName = path + _T("\\") + logList[i].fileName;
			std::tstring newName = curName;
			TCHAR buffer[12];
			_stprintf(buffer, _T("%.2d"), i - 1);
			newName.replace(newName.find(_T(".txt")) - 2, 2, buffer);

			FileMisc::MoveOrRename(curName.c_str(), newName.c_str());
			logList[i].fileName = newName.substr(newName.find_last_of(_T("\\")) + 1);
		}

		logList.erase(logList.begin() + index);
	}

	void ClearLogs()
	{
		for(int i = 0, size = logList.size(); i < size; i++)
			FileMisc::Remove((path + _T("\\") + logList[i].fileName).c_str());

		logList.clear();
	}

	std::vector<FileMisc::FileData>& GetLogList()
	{
		return logList;
	}
private:
	std::vector<FileMisc::FileData> logList;
	std::tstring path;
};
