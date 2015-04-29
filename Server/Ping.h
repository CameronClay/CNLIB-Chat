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

		PingData& operator=(PingData&& data)
		{
			//serv = std::move(data.serv);
			serv = std::forward<TCPServ>(data.serv);
			pingHandler = data.pingHandler;
			socket = data.socket;
			ZeroMemory(&data, sizeof(PingData));
			return *this;
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

		PingDataEx& operator=(PingDataEx&& data)
		{
			*(PingData*)this = (PingData)data;
			const_cast<float&>(inactiveInterval) = data.inactiveInterval;
			const_cast<float&>(pingInterval) = data.pingInterval;
			return *this;
		}

		const float inactiveInterval, pingInterval;
	};

	PingHandler& operator=(PingHandler&& data)
	{
		recvThread = data.recvThread;
		inactivityThread = data.inactivityThread;
		inactivityTimer = data.inactivityTimer;
		sendPingThread = data.sendPingThread;
		sendPingTimer = data.sendPingTimer;
		pingDataEx = data.pingDataEx;
		ZeroMemory(&data, sizeof(PingHandler));
		return *this;
	}

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