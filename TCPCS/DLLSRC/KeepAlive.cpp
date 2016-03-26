#include "StdAfx.h"
#include "KeepAlive.h"
#include "CNLIB/HeapAlloc.h"

DWORD CALLBACK KeepAliveThread(LPVOID info)
{
	KeepAliveHandler::KeepAliveData& datRef = *(KeepAliveHandler::KeepAliveData*)info;
	HANDLE timer = datRef.keepAliveHandler->GetKeepAliveTimer();

	while(true)
	{
		const DWORD ret = MsgWaitForMultipleObjects(1, &timer, FALSE, INFINITE, QS_ALLINPUT);
		if (ret == WAIT_OBJECT_0)
			datRef.keepAliveHI->KeepAlive();

		else if(ret == WAIT_OBJECT_0 + 1)
		{
			MSG msg;
			while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) != 0)
			{
				if(msg.message == WM_QUIT)
					return 0;
			}
		}

		else
			return 0;
	}

	return 0;
}

KeepAliveHandler::KeepAliveData::KeepAliveData(KeepAliveHI* keepAliveHI, KeepAliveHandler* keepAliveHandler)
	:
	keepAliveHI(keepAliveHI),
	keepAliveHandler(keepAliveHandler)
{}

KeepAliveHandler::KeepAliveData::KeepAliveData(KeepAliveData&& keepAliveData)
	:
	keepAliveHI(keepAliveData.keepAliveHI),
	keepAliveHandler(keepAliveData.keepAliveHandler)
{
	ZeroMemory(&keepAliveData, sizeof(KeepAliveData));
}


auto KeepAliveHandler::KeepAliveData::operator=(KeepAliveData&& data) -> KeepAliveData&
{
	if(this != &data)
	{
		keepAliveHI = data.keepAliveHI;
		keepAliveHandler = data.keepAliveHandler;

		ZeroMemory(&data, sizeof(KeepAliveData));
	}
	return *this;
}


KeepAliveHandler::KeepAliveHandler(KeepAliveHI* keepAliveHI)
	:
	keepAliveTimer(CreateWaitableTimer(NULL, FALSE, NULL)),
	keepAliveThread(NULL),
	keepAliveID(NULL),
	keepAliveData(construct<KeepAliveData>(keepAliveHI, this))
{}

KeepAliveHandler::KeepAliveHandler(KeepAliveHandler&& keepAlive)
	:
	keepAliveTimer(keepAlive.keepAliveTimer),
	keepAliveThread(keepAlive.keepAliveThread),
	keepAliveID(keepAlive.keepAliveID),
	keepAliveData(keepAlive.keepAliveData)
{
	keepAliveData->keepAliveHandler = this;
	ZeroMemory(&keepAlive, sizeof(KeepAliveHandler));
}

KeepAliveHandler& KeepAliveHandler::operator=(KeepAliveHandler&& data)
{
	if(this != &data)
	{
		this->~KeepAliveHandler();
		keepAliveTimer = data.keepAliveTimer;
		keepAliveThread = data.keepAliveThread;
		keepAliveID = data.keepAliveID;
		keepAliveData = data.keepAliveData;
		keepAliveData->keepAliveHandler = this;

		ZeroMemory(&data, sizeof(KeepAliveHandler));
	}
	return *this;
}

KeepAliveHandler& KeepAliveHandler::operator=(const KeepAliveHandler& data)
{
	if(this != &data)
	{
		this->~KeepAliveHandler();
		keepAliveTimer = data.keepAliveTimer;
		keepAliveThread = data.keepAliveThread;
		keepAliveID = data.keepAliveID;
		keepAliveData = data.keepAliveData;
		keepAliveData->keepAliveHandler = this;
	}
	return *this;
}

KeepAliveHandler::~KeepAliveHandler()
{
	if(keepAliveTimer)
	{
		CancelWaitableTimer(keepAliveTimer);
		CloseHandle(keepAliveTimer);
		keepAliveTimer = NULL;
	}

	if(keepAliveThread)
	{
		PostThreadMessage(keepAliveID, WM_QUIT, 0, 0);
		WaitForSingleObject(keepAliveThread, INFINITE);
		CloseHandle(keepAliveThread);
		keepAliveThread = NULL;
	}

	destruct(keepAliveData);
}

void KeepAliveHandler::SetKeepAliveTimer(float interval)
{
	LARGE_INTEGER LI;
	LI.QuadPart = (LONGLONG)(interval * -10000000.0f);
	SetWaitableTimer(keepAliveTimer, &LI, (LONG)(interval * 1000.0f), NULL, NULL, FALSE);
	if(!keepAliveThread)
		keepAliveThread = CreateThread(NULL, 0, KeepAliveThread, keepAliveData, NULL, &keepAliveID);
}

HANDLE KeepAliveHandler::GetKeepAliveTimer() const
{
	return keepAliveTimer;
}
