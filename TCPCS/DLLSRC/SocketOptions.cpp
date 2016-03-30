#include "stdafx.h"
#include "CNLIB/SocketOptions.h"

SocketOptions::SocketOptions(bool useOwnSBuf, bool useOwnRBuf, bool noDelay)
	:
	useOwnSBuf(useOwnSBuf),
	useOwnRBuf(useOwnRBuf),
	noDelay(noDelay)
{}
SocketOptions& SocketOptions::operator=(const SocketOptions& sockOpts)
{
	if (this != &sockOpts)
	{
		useOwnSBuf = sockOpts.useOwnSBuf;
		useOwnRBuf = sockOpts.useOwnRBuf;
		noDelay = sockOpts.noDelay;
	}
	return *this;
}
bool SocketOptions::UseOwnSBuf() const
{
	return useOwnSBuf;
}
bool SocketOptions::UseOwnRBuf() const
{
	return useOwnRBuf;
}
bool SocketOptions::NoDelay() const
{
	return noDelay;
}