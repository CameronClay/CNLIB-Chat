//Copyright (c) <2015> <Cameron Clay>

#pragma once
#include "TCPClient.h"

class VerifyPing //used to check if server loses net or is shutdown ungracefully
{
public:
	struct VerifyData
	{
		VerifyData(TCPClient& client, VerifyPing* verify)
			:
			client(client),
			verify(verify)
		{}

		TCPClient& client;
		VerifyPing* verify;
	};

	VerifyPing(TCPClient& client);
	VerifyPing(VerifyPing&& verifyPing);
	~VerifyPing();

	HANDLE GetTimer() const;
	void SetTimer(float interval);
private:
	HANDLE timer, thread;
	DWORD threadID;
	VerifyData* verifyData;
};