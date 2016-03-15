#pragma once
#include "Typedefs.h"

class CAMSNETLIB SocketOptions
{
public:
	SocketOptions(bool useOwnBuf = true, bool noDelay = false);
	SocketOptions& operator=(const SocketOptions& sockOpts);
	bool UseOwnBuf() const;
	bool NoDelay() const;
private:
	bool useOwnBuf, noDelay;
};