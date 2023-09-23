#include "Stdafx.h"
#include "CNLIB/SocketOptions.h"

SocketOptions::SocketOptions(int tcpSendSize, int tcpRecvSize, bool noDelay)
	:
	tcpSendSize(tcpSendSize),
	tcpRecvSize(tcpRecvSize),
	noDelay(noDelay)
{}
SocketOptions& SocketOptions::operator=(const SocketOptions& sockOpts)
{
	if (this != &sockOpts)
	{
		tcpSendSize = sockOpts.tcpSendSize;
		tcpRecvSize = sockOpts.tcpRecvSize;
		noDelay = sockOpts.noDelay;
	}
	return *this;
}
int SocketOptions::TCPSendSize() const
{
	return tcpSendSize;
}
int SocketOptions::TCPRecvSize() const
{
	return tcpRecvSize;
}
bool SocketOptions::NoDelay() const
{
	return noDelay;
}