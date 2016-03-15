#include "stdafx.h"
#include "CNLIB/SocketOptions.h"

SocketOptions::SocketOptions(bool useOwnBuf, bool noDelay)
	:
	useOwnBuf(useOwnBuf),
	noDelay(noDelay)
{}
SocketOptions& SocketOptions::operator=(const SocketOptions& sockOpts)
{
	if (this != &sockOpts)
	{
		useOwnBuf = sockOpts.useOwnBuf;
		noDelay = sockOpts.noDelay;
	}
	return *this;
}
bool SocketOptions::UseOwnBuf() const
{
	return useOwnBuf;
}
bool SocketOptions::NoDelay() const
{
	return noDelay;
}