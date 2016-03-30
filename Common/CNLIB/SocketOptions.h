#pragma once
#include "Typedefs.h"

class CAMSNETLIB SocketOptions
{
public:
	SocketOptions(bool useOwnSBuf = true, bool useOwnRBuf = false, bool noDelay = false);
	SocketOptions& operator=(const SocketOptions& sockOpts);
	bool UseOwnSBuf() const;
	bool UseOwnRBuf() const;
	bool NoDelay() const;
private:
	bool useOwnSBuf, useOwnRBuf, noDelay;
};