#pragma once
#include "Socket.h"

class TCPServ;

class PingHandler
{
public:
	struct PingData
	{
		PingData(TCPServ& serv, PingHandler& pingHandler, Socket& socket);
		PingData(PingData&& pingData);
		PingData& operator=(PingData&& data);

		TCPServ& serv;
		PingHandler& pingHandler;
		Socket& socket;
	};
	struct PingDataEx : PingData
	{
		PingDataEx(TCPServ& serv, PingHandler& pingHandler, Socket& socket, float inactiveInterval, float pingInterval);
		PingDataEx(PingDataEx&& pingDataEx);
		PingDataEx& operator=(PingDataEx&& data);

		const float inactiveInterval, pingInterval;
	};

	PingHandler& operator=(PingHandler&& data);

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