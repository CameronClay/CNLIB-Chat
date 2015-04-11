#pragma once
#include <netfw.h>
#include <Windows.h>


class WinFirewall
{
public:
	WinFirewall();
	~WinFirewall();
	bool Initialize();
	//																							NET_FW_IP_PROTOCOL_TCP or NET_FW_IP_PROTOCOL_UDP, NET_FW_ACTION_ALLOW or NET_FW_ACTION_BLOCK
	bool AddException(const TCHAR* exceptionName, const TCHAR* description, const TCHAR* appName, const TCHAR* serviceName, NET_FW_IP_PROTOCOL protocal, const TCHAR* port, NET_FW_ACTION action, bool enabled);
	bool ItemExists(const TCHAR* name);
	bool RemoveItem(const TCHAR* name);
private:
	INetFwRules *nfrs;
	long curProfs;
};