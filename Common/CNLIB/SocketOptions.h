#pragma once
#include "Typedefs.h"

class CAMSNETLIB SocketOptions
{
public:
	SocketOptions(int tcpSendSize = 0, int tcpRecvSize = 65536, bool noDelay = true);
	SocketOptions& operator=(const SocketOptions& sockOpts);
	int TCPSendSize() const;
	int TCPRecvSize() const;
	bool NoDelay() const;
private:
	int tcpSendSize, tcpRecvSize;
	bool noDelay;
};