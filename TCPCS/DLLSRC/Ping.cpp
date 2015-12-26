#include "Ping.h"
#include "TCPServ.h"
#include "HeapAlloc.h"
#include "Messages.h"

DWORD CALLBACK PingThread(LPVOID info)
{
	PingHandler::PingData& datRef = *(PingHandler::PingData*)info;
	HANDLE timer = datRef.pingHandler->GetPingTimer();

	while(true)
	{
		const DWORD ret = MsgWaitForMultipleObjects(1, &timer, FALSE, INFINITE, QS_ALLINPUT);
		if (ret == WAIT_OBJECT_0)
			datRef.pingHI->Ping();

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

PingHandler::PingData::PingData(PingHI* pingHI, PingHandler* pingHandler)
	:
	pingHI(pingHI),
	pingHandler(pingHandler)
{}

PingHandler::PingData::PingData(PingData&& pingData)
	:
	pingHI(pingData.pingHI),
	pingHandler(pingData.pingHandler)
{
	ZeroMemory(&pingData, sizeof(PingData));
}


auto PingHandler::PingData::operator=(PingData&& data) -> PingData&
{
	if(this != &data)
	{
		pingHI = data.pingHI;
		pingHandler = data.pingHandler;

		ZeroMemory(&data, sizeof(PingData));
	}
	return *this;
}


PingHandler::PingHandler(PingHI* pingHI)
	:
	pingTimer(CreateWaitableTimer(NULL, FALSE, NULL)),
	pingThread(NULL),
	pingID(NULL),
	pingData(construct<PingData>(pingHI, this))
{}

PingHandler::PingHandler(PingHandler&& ping)
	:
	pingTimer(ping.pingTimer),
	pingThread(ping.pingThread),
	pingID(ping.pingID),
	pingData(ping.pingData)
{
	pingData->pingHandler = this;
	ZeroMemory(&ping, sizeof(PingHandler));
}

PingHandler& PingHandler::operator=(PingHandler&& data)
{
	if(this != &data)
	{
		this->~PingHandler();
		pingTimer = data.pingTimer;
		pingThread = data.pingThread;
		pingID = data.pingID;
		pingData = data.pingData;
		pingData->pingHandler = this;

		ZeroMemory(&data, sizeof(PingHandler));
	}
	return *this;
}

PingHandler& PingHandler::operator=(const PingHandler& data)
{
	if(this != &data)
	{
		this->~PingHandler();
		pingTimer = data.pingTimer;
		pingThread = data.pingThread;
		pingID = data.pingID;
		pingData = data.pingData;
		pingData->pingHandler = this;
	}
	return *this;
}

PingHandler::~PingHandler()
{
	if(pingTimer)
	{
		CancelWaitableTimer(pingTimer);
		CloseHandle(pingTimer);
		pingTimer = NULL;
	}

	if(pingThread)
	{
		PostThreadMessage(pingID, WM_QUIT, 0, 0);
		WaitForSingleObject(pingThread, INFINITE);
		CloseHandle(pingThread);
		pingThread = NULL;
	}

	destruct(pingData);
}

void PingHandler::SetPingTimer(float interval)
{
	LARGE_INTEGER LI;
	LI.QuadPart = (LONGLONG)(interval * -10000000.0f);
	SetWaitableTimer(pingTimer, &LI, (LONG)(interval * 1000.0f), NULL, NULL, FALSE);
	if(!pingThread)
		pingThread = CreateThread(NULL, 0, PingThread, pingData, NULL, &pingID);
}

HANDLE PingHandler::GetPingTimer() const
{
	return pingTimer;
}
