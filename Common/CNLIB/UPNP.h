#pragma once
#include "Typedefs.h"
#include <Natupnp.h>
#include <Windows.h>
#include <tchar.h>

// "TCP" or "UDP" for protocal
class CAMSNETLIB UPNP
{
public:
	UPNP();
	~UPNP();
	bool Initialize();
	//returns true if it exists and succeeds
	bool PortMapExists(USHORT port, const LIB_TCHAR* protocal) const;
	bool RemovePortMap(USHORT port, const LIB_TCHAR* protocal);
	bool AddPortMap(USHORT port, const LIB_TCHAR* protocal, const LIB_TCHAR* ip, const LIB_TCHAR* description, bool state = true);
	long GetPortMapCount();
private:
	IStaticPortMappingCollection *spmc;
};

CAMSNETLIB bool MapPort(USHORT port, const LIB_TCHAR* protocal, const LIB_TCHAR* name);
