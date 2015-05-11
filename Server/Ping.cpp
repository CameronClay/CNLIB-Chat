#include "Ping.h"
#include "TCPServ.h"
#include "HeapAlloc.h"
#include "Messages.h"

PingHandler::PingData::PingData(TCPServ& serv, PingHandler& pingHandler, Socket& socket)
	:
	pingHandler(pingHandler),
	serv(serv),
	socket(socket)
{}

PingHandler::PingData::PingData(PingData&& pingData)
	:
	pingHandler(std::move(pingData.pingHandler)),
	serv(std::move(pingData.serv)),
	socket(std::move(pingData.socket))
{
	ZeroMemory(&pingData, sizeof(PingData));
}


auto PingHandler::PingData::operator=(PingData&& data) -> PingData&
{
	if(this != &data)
	{
		serv = std::move(data.serv);
		pingHandler = std::move(data.pingHandler);
		socket = std::move(data.socket);

		ZeroMemory(&data, sizeof(PingData));
	}
	return *this;
}


PingHandler::PingDataEx::PingDataEx(TCPServ& serv, PingHandler& pingHandler, Socket& socket, float inactiveInterval, float pingInterval)
	:
	PingData(serv, pingHandler, socket),
	inactiveInterval(inactiveInterval),
	pingInterval(pingInterval)
{}

PingHandler::PingDataEx::PingDataEx(PingDataEx&& pingDataEx)
	:
	PingData(std::forward<PingData>(pingDataEx)),
	inactiveInterval(pingDataEx.inactiveInterval),
	pingInterval(pingDataEx.pingInterval)
{
	ZeroMemory(&pingDataEx, sizeof(PingDataEx));
}

auto PingHandler::PingDataEx::operator=(PingDataEx&& data) -> PingDataEx&
{
	if(this != &data)
	{
		*(PingData*)this = (PingData)data;
		const_cast<float&>(inactiveInterval) = data.inactiveInterval;
		const_cast<float&>(pingInterval) = data.pingInterval;

		ZeroMemory(&data, sizeof(PingDataEx));
	}
	return *this;
}

PingHandler::PingHandler()
	:
	inactivityThread(NULL),
	inactivityTimer(CreateWaitableTimer(NULL, FALSE, NULL)),
	sendPingThread(NULL),
	sendPingTimer(CreateWaitableTimer(NULL, FALSE, NULL)),
	sendPingID(NULL),
	inactivityID(NULL),
	pingDataEx(nullptr)
{}

PingHandler::PingHandler(PingHandler&& ping)
	:
	recvThread(ping.recvThread),
	inactivityThread(ping.inactivityThread),
	inactivityTimer(ping.inactivityTimer),
	sendPingThread(ping.sendPingThread),
	sendPingTimer(ping.sendPingTimer),
	sendPingID(ping.sendPingID),
	inactivityID(ping.inactivityID),
	pingDataEx(ping.pingDataEx)
{
	ZeroMemory(&ping, sizeof(PingHandler));
}

PingHandler& PingHandler::operator=(PingHandler&& data)
{
	if(this != &data)
	{
		this->~PingHandler();
		recvThread = data.recvThread;
		inactivityThread = data.inactivityThread;
		inactivityTimer = data.inactivityTimer;
		sendPingThread = data.sendPingThread;
		sendPingTimer = data.sendPingTimer;
		sendPingID = data.sendPingID;
		inactivityID = data.inactivityID;
		pingDataEx = data.pingDataEx;

		ZeroMemory(&data, sizeof(PingHandler));
	}
	return *this;
}

PingHandler::~PingHandler()
{
	if(inactivityTimer)
	{
		CancelWaitableTimer(inactivityTimer);
		CloseHandle(inactivityTimer);
		inactivityTimer = NULL;
	}

	if(sendPingTimer)
	{
		CancelWaitableTimer(sendPingTimer);
		CloseHandle(sendPingTimer);
		sendPingTimer = NULL;
	}

	if(inactivityThread)
	{
		PostThreadMessage(inactivityID, WM_QUIT, 0, 0);
		CloseHandle(inactivityThread);
		inactivityThread = NULL;
	}

	if(sendPingThread)
	{
		PostThreadMessage(sendPingID, WM_QUIT, 0, 0);
		CloseHandle(sendPingThread);
		sendPingThread = NULL;
	}

	destruct(pingDataEx);
}

DWORD CALLBACK Inactivity(LPVOID info)
{
	PingHandler::PingDataEx& data = *(PingHandler::PingDataEx*)info;
	HANDLE timer = data.pingHandler.GetInactivityTimer();

	while(true)
	{
		const DWORD ret = MsgWaitForMultipleObjects(1, &timer, FALSE, INFINITE, QS_ALLINPUT);
		if(ret == WAIT_OBJECT_0)
			data.pingHandler.AutoPingHandlerOn(data);

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

DWORD CALLBACK PingThread(LPVOID info)
{
	PingHandler::PingData& datRef = *(PingHandler::PingData*)info;
	HANDLE timer = datRef.pingHandler.GetPingTimer();

	while(true)
	{
		const DWORD ret = MsgWaitForMultipleObjects(1, &timer, FALSE, INFINITE, QS_ALLINPUT);
		if(ret == WAIT_OBJECT_0)
			datRef.serv.Ping(datRef.socket);

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

void PingHandler::AutoPingHandlerOn(PingDataEx& pingData)
{
	LARGE_INTEGER LI;
	LI.QuadPart = 0;
	BOOL res = SetWaitableTimer(sendPingTimer, &LI, (LONG)(pingData.pingInterval * 1000.0f), NULL, NULL, FALSE);
	if(!sendPingThread)
		sendPingThread = CreateThread(NULL, 0, PingThread, (LPVOID)((PingData*)&pingData), NULL, &sendPingID);
}

void PingHandler::SetInactivityTimer(TCPServ& serv, Socket& socket, float inactiveInterval, float pingInterval)
{
	//[&sendPingTimer, &sendPingThread]()
	[this]()//Turns Autoping off
	{
		CancelWaitableTimer(sendPingTimer);

		if(sendPingThread)
		{
			PostThreadMessage(sendPingID, WM_QUIT, 0, 0);
			CloseHandle(sendPingThread);
			sendPingThread = NULL;
		}
	}();

	LARGE_INTEGER LI;
	LI.QuadPart = (LONGLONG)(inactiveInterval * -10000000.0f);
	BOOL res = SetWaitableTimer(inactivityTimer, &LI, (LONG)(inactiveInterval * 1000.0f), NULL, NULL, FALSE);
	if(!inactivityThread)
	{
		destruct(pingDataEx);
		pingDataEx = construct<PingDataEx>(PingDataEx(serv, *this, socket, inactiveInterval, pingInterval));
		inactivityThread = CreateThread(NULL, 0, Inactivity, (LPVOID)pingDataEx, NULL, &inactivityID);
	}
}

HANDLE PingHandler::GetInactivityTimer() const
{
	return inactivityTimer;
}

HANDLE PingHandler::GetPingTimer() const
{
	return sendPingTimer;
}

