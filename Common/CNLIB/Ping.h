//Copyright (c) <2015> <Cameron Clay>

#pragma once
#include "Socket.h"

class TCPServ;

class CAMSNETLIB PingHI
{
public:
	virtual void Ping() = 0;
	virtual void SetPingInterval(float interval) = 0;
	virtual float GetPingInterval() const = 0;
};

class CAMSNETLIB PingHandler
{
public:
	struct CAMSNETLIB PingData
	{
		PingData(PingHI* pingHI, PingHandler* pingHandler);
		PingData(PingData&& pingData);
		PingData& operator=(PingData&& data);

		PingHI* pingHI;
		PingHandler* pingHandler;
	};

	PingHandler& operator=(PingHandler&& data);
	PingHandler& operator=(const PingHandler& data);

	PingHandler(PingHI* pingHI);
	PingHandler(PingHandler&& ping);
	~PingHandler();

	void SetPingTimer(float interval);

	HANDLE GetPingTimer() const;

private:	
	HANDLE pingTimer, pingThread;
	DWORD pingID;
	PingData* pingData;
};