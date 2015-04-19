#pragma once

#include "Typedefs.h"
#include <ws2tcpip.h>
#include <Mstcpip.h>
#include <Windows.h>
#include <stdlib.h>
#include <vector>
#include <functional>
#include <tchar.h>

#if NTDDI_VERSION >= NTDDI_VISTA
#include <Natupnp.h>
#pragma comment(lib,"Ntdll.lib")
#endif


typedef void(*sfunc)(void* manager, USHORT& index, BYTE* data, DWORD nBytes, void* obj);
typedef void(**const sfuncP)(void* manager, USHORT& index, BYTE* data, DWORD nBytes, void* obj);
typedef void(*cfunc)(void* manager, BYTE* data, DWORD nBytes, void* obj);
typedef void(**const cfuncP)(void* manager, BYTE* data, DWORD nBytes, void* obj);

void InitializeNetworking();
void CleanupNetworking();

class Socket
{
public:
	Socket(const TCHAR* port);
	Socket(SOCKET pc);
	Socket();
	Socket(Socket&& sock);
	~Socket();

	Socket& operator= (Socket& pc);
	bool operator== (Socket& pc) const;
	bool operator!= (Socket& pc) const;
	bool operator== (SOCKET pc) const;
	bool operator!= (SOCKET pc) const;

	void Bind(const TCHAR* port);
	Socket AcceptConnection();

	void Connect(const TCHAR* dest, const TCHAR* port, float timeout);
	void Disconnect();

	long ReadData(void* dest, DWORD nBytes);
	long SendData(const void* data, DWORD nBytes);

	void ToIp(TCHAR* ipaddr) const;
	bool IsConnected() const;

	//dest size needs to be at least 16
	static void GetLocalIP(TCHAR* dest);
	static void HostNameToIP(const TCHAR* host, TCHAR* dest, UINT buffSize);

private:
	SOCKET pc;
};
