#pragma once
#include "CNLIB\Typedefs.h"
#include <netfw.h>
#include <Windows.h>


class WinFirewall
{
public:
	WinFirewall();
	~WinFirewall();
	bool Initialize();
	//																							NET_FW_IP_PROTOCOL_TCP or NET_FW_IP_PROTOCOL_UDP, NET_FW_ACTION_ALLOW or NET_FW_ACTION_BLOCK
	bool AddException(const LIB_TCHAR* exceptionName, const LIB_TCHAR* description, const LIB_TCHAR* appName, const LIB_TCHAR* serviceName, NET_FW_IP_PROTOCOL protocal, const LIB_TCHAR* port, NET_FW_ACTION action, bool enabled);
	bool ItemExists(const LIB_TCHAR* name);
	bool RemoveItem(const LIB_TCHAR* name);
private:
	INetFwRules *nfrs;
	long curProfs;
};