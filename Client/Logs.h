#pragma once
#include "CNLIB/Typedefs.h"
#include "CNLIB/File.h"
#include "CNLIB/HeapAlloc.h"
#include <algorithm>

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
        std::tstring newpath = path;
        std::replace(std::begin(newpath), std::end(newpath), '/', '\\');
        this->path = newpath;

		logList = FileMisc::GetFileNameList(path.c_str(), FILE_ATTRIBUTE_TEMPORARY, false);
        CompressTempLogs(FileMisc::GetFileNameList(path.c_str(), FILE_ATTRIBUTE_TEMPORARY, true));
		currLog = logList.size();
	}

	void CreateLog()
	{
		TCHAR buffer[512];
        _stprintf(buffer, _T("%s\\Log%.2d.txt"), path.c_str(), currLog);
		fileName = buffer;

		log.Open(buffer, FILE_GENERIC_WRITE, FILE_ATTRIBUTE_TEMPORARY, CREATE_ALWAYS);
	}

	void SaveLog()
	{
		if (log.IsOpen())
        {
            if (log.GetCursor() != 0) {
                CompressTempLog(log, fileName);
            }
            else {
                RemoveTempLog();
            }
		}
	}

	void WriteLine(const std::tstring& str)
	{
		if (log.IsOpen())
		{
			std::tstring temp;
			if (log.GetCursor() != 0)
				temp.append(_T("\r\n"));

			temp.append(str);

			log.Write(temp.c_str(), temp.size() * sizeof(LIB_TCHAR));
		}
	}

	//if nBytes is specified copies that many bytes into dest, otherwise outputs the number of bytes into the nBytes var
	void ReadLog(int index, char* dest = nullptr, DWORD* nBytes = nullptr)
	{
        File log((path + _T("\\") + logList[index].fileName).c_str(), FILE_GENERIC_READ);

		if (dest)
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
            log.Read((void*)nBytes, sizeof(DWORD));
		}
	}

	void RemoveTempLog()
	{
		log.Close();
		FileMisc::Remove(fileName.c_str());
	}

	void RemoveLog(int index)
	{
        FileMisc::Remove((path + _T("\\") + logList[index].fileName).c_str());

		//change all log's index above the removed log -1
		for (int i = index + 1, size = logList.size(); i < size; i++)
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
		for (int i = 0, size = logList.size(); i < size; i++)
            FileMisc::Remove((path + _T("\\") + logList[i].fileName).c_str());

		logList.clear();
	}

	std::vector<FileMisc::FileData>& GetLogList()
	{
		return logList;
	}
private:
	FileMisc::FileData CompressTempLog(const FileMisc::FileData& fd)
	{
		std::tstring logName;
		if (fileName == fd.fileName)
		{
			logName = fd.fileName;
			log.Close();
		}
		else
		{
            logName = path + _T("\\") + fd.fileName;
		}

		File log(logName.c_str(), FILE_GENERIC_READ, FILE_FLAG_DELETE_ON_CLOSE);
		while (!log.IsOpen())
		{
			TCHAR buffer[512];
            _stprintf(buffer, _T("%s\\Log%.2d.txt"), path.c_str(), ++currLog);
			fileName = logName = buffer;
			log.Open(logName.c_str(), FILE_GENERIC_READ, FILE_FLAG_DELETE_ON_CLOSE);
		}
		const DWORD tempLogSize = (DWORD)log.GetSize() + sizeof(LIB_TCHAR);
		if (tempLogSize == sizeof(LIB_TCHAR))
			return{};

		DWORD compLogSize = FileMisc::GetCompressedBufferSize((DWORD)tempLogSize);
		char* buffer = alloc<char>(tempLogSize + compLogSize);
		char* newBuffer = buffer + tempLogSize;

		*(LIB_TCHAR*)(buffer + tempLogSize - sizeof(LIB_TCHAR)) = _T('\0');

		log.Read(buffer, tempLogSize);
		compLogSize = FileMisc::Compress((BYTE*)newBuffer, compLogSize, (BYTE*)buffer, tempLogSize, 9);
		log.Close();

		//*(LIB_TCHAR*)(newBuffer + compLogSize + sizeof(LIB_TCHAR)) = _T('\0');

		log.Open(logName.c_str(), FILE_GENERIC_WRITE, FILE_ATTRIBUTE_NORMAL, CREATE_ALWAYS);
		log.Write((void*)&tempLogSize, sizeof(DWORD));
		log.Write((void*)&compLogSize, sizeof(DWORD));
		log.Write(newBuffer, compLogSize);
		log.ChangeDate(fd.dateModified);

		dealloc(buffer);

		return{ fd.fileName, fd.dateModified, log.GetSize() };
	}
	FileMisc::FileData CompressTempLog(const File& file, const std::tstring& fileName)
	{
		return CompressTempLog({ fileName, file.GetDate(), file.GetSize() });
	}
	void CompressTempLogs(const std::vector<FileMisc::FileData>& fileList)
	{
		for (auto& it : fileList)
		{
			const FileMisc::FileData fd = CompressTempLog(it);
			if (fd.Valid())
				logList.push_back(std::move(fd));
		}
	}

	std::vector<FileMisc::FileData> logList;
	File log;
	UINT currLog;
	std::tstring fileName;
	std::tstring path;
};
