#include "Ping.h"
#include "TCPServ.h"
#include "HeapAlloc.h"
#include "Messages.h"

PingHandler::PingHandler()
	:
	inactivityThread(NULL),
	inactivityTimer(CreateWaitableTimer(NULL, FALSE, NULL)),
	sendPingThread(NULL),
	sendPingTimer(CreateWaitableTimer(NULL, FALSE, NULL))
{}

PingHandler::PingHandler(PingHandler&& ping)
	:
	recvThread(ping.recvThread),
	inactivityThread(ping.inactivityThread),
	inactivityTimer(ping.inactivityTimer),
	sendPingThread(ping.sendPingThread),
	sendPingTimer(ping.sendPingTimer)
{
	ZeroMemory(&ping, sizeof(PingHandler));
}

PingHandler::~PingHandler()
{
	if(inactivityTimer)
	{
		CancelWaitableTimer(inactivityTimer);
		CloseHandle(inactivityTimer);
		inactivityTimer = NULL;
	}

	if(inactivityThread)
	{
		//TerminateThread(inactivityThread, 0);
		CloseHandle(inactivityThread);
		inactivityThread = NULL;
	}

	if(sendPingTimer)
	{
		CancelWaitableTimer(sendPingTimer);
		CloseHandle(sendPingTimer);
		sendPingTimer = NULL;
	}

	if(sendPingThread)
	{
		//TerminateThread(sendPingThread, 0);
		CloseHandle(sendPingThread);
		sendPingThread = NULL;
	}
}

DWORD CALLBACK Inactivity(LPVOID info)
{
	PingHandler::PingDataEx* data = (PingHandler::PingDataEx*)info;
	PingHandler::PingDataEx& dat = *data;

	while(WaitForSingleObject(dat.pingHandler.GetInactivityTimer(), INFINITE) == WAIT_OBJECT_0)
	{
		dat.pingHandler.AutoPingHandlerOn(dat);
	}

	destruct(data);

	return 0;
}

DWORD CALLBACK PingThread(LPVOID info)
{
	PingHandler::PingData& datRef = *(PingHandler::PingData*)info;

	while(WaitForSingleObject(datRef.pingHandler.GetPingTimer(), INFINITE) == WAIT_OBJECT_0)
	{
		datRef.serv.Ping(datRef.socket);
	}

	return 0;
}

void PingHandler::AutoPingHandlerOn(PingDataEx& pingData)
{
	LARGE_INTEGER LI;
	LI.QuadPart = (LONGLONG)(pingData.pingInterval * -10000000.0f);
	BOOL res = SetWaitableTimer(sendPingTimer, &LI, (LONG)(pingData.pingInterval * 1000.0f), NULL, NULL, FALSE);
	if(!sendPingThread)
		sendPingThread = CreateThread(NULL, 0, PingThread, (LPVOID)((PingData*)&pingData), NULL, NULL);
}

void PingHandler::SetInactivityTimer(TCPServ& serv, Socket& socket, float inactiveInterval, float pingInterval)
{
	//[&sendPingTimer, &sendPingThread]()
	[this]()//Turns Autoping off
	{
		CancelWaitableTimer(sendPingTimer);

		if(sendPingThread)
		{
			CloseHandle(sendPingThread);
			sendPingThread = NULL;
		}
	}();

	LARGE_INTEGER LI;
	LI.QuadPart = (LONGLONG)(inactiveInterval * -10000000.0f);
	BOOL res = SetWaitableTimer(inactivityTimer, &LI, (LONG)(inactiveInterval * 1000.0f), NULL, NULL, FALSE);
	if(!inactivityThread)
		inactivityThread = CreateThread(NULL, 0, Inactivity, (LPVOID)construct<PingDataEx>(PingDataEx(serv, *this, socket, inactiveInterval, pingInterval)), NULL, NULL);
}

HANDLE PingHandler::GetInactivityTimer() const
{
	return inactivityTimer;
}

HANDLE PingHandler::GetPingTimer() const
{
	return sendPingTimer;
}
