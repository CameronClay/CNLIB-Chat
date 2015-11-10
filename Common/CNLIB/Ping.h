//Copyright (c) <2015> <Cameron Clay>

#pragma once
#include "Socket.h"

class TCPServ;

class CAMSNETLIB PingHandler
{
public:
	struct CAMSNETLIB PingData
	{
		PingData(TCPServ& serv, PingHandler* pingHandler);
		PingData(PingData&& pingData);
		PingData& operator=(PingData&& data);

		TCPServ& serv;
		PingHandler* pingHandler;
	};

	PingHandler& operator=(PingHandler&& data);
	PingHandler& operator=(const PingHandler& data);

	PingHandler(TCPServ& serv);
	PingHandler(PingHandler&& ping);
	~PingHandler();

	void SetPingTimer(float interval);

	HANDLE GetPingTimer() const;

private:	
	HANDLE pingTimer, pingThread;
	DWORD pingID;
	PingData* pingData;
};