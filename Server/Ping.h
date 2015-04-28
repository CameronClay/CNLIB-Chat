#pragma once
#include "Socket.h"

class TCPServ;

class PingHandler
{
public:
	struct PingData
	{
		PingData(TCPServ& serv, PingHandler& pingHandler, Socket& socket)
			:
			pingHandler(pingHandler),
			serv(serv),
			socket(socket)
		{}
		PingData(PingData&& pingData)
			:
			pingHandler(pingData.pingHandler),
			serv(pingData.serv),
			socket(pingData.socket)
		{
			ZeroMemory(&pingData, sizeof(PingData));
		}

		TCPServ& serv;
		PingHandler& pingHandler;
		Socket& socket;
	};
	struct PingDataEx : PingData
	{
		PingDataEx(TCPServ& serv, PingHandler& pingHandler, Socket& socket, float inactiveInterval, float pingInterval)
			:
			PingData(serv, pingHandler, socket),
			inactiveInterval(inactiveInterval),
			pingInterval(pingInterval)
		{}
		PingDataEx(PingDataEx&& pingDataEx)
			:
			PingData(std::forward<PingData>(pingDataEx)),
			inactiveInterval(pingDataEx.inactiveInterval),
			pingInterval(pingDataEx.pingInterval)
		{
			ZeroMemory(&pingDataEx, sizeof(PingDataEx));
		}

		const float inactiveInterval, pingInterval;
	};

	PingHandler();
	PingHandler(PingHandler&& ping);
	~PingHandler();

	void SetInactivityTimer(TCPServ& serv, Socket& socket, float inactiveInterval, float pingInterval);

	HANDLE GetInactivityTimer() const;
	HANDLE GetPingTimer() const;

	void AutoPingHandlerOn(PingDataEx& pingData);

private:
	HANDLE recvThread, inactivityThread, inactivityTimer, sendPingThread, sendPingTimer;
	PingDataEx* pingDataEx;
};