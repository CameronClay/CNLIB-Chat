//Copyright (c) <2015> <Cameron Clay>

#pragma once

#include "Typedefs.h"
#include <tchar.h>

#if NTDDI_VERSION >= NTDDI_VISTA
#include <Natupnp.h>
#pragma comment(lib,"Ntdll.lib")
#endif

CAMSNETLIB int InitializeNetworking();
CAMSNETLIB int CleanupNetworking();

class CAMSNETLIB Socket
{
public:
	Socket(const LIB_TCHAR* port);
	Socket(SOCKET pc);
	Socket();
	Socket(Socket&& sock);
	~Socket(); //does not disconnect socket

	struct Hash
	{
		size_t operator()(const Socket& sock) const
		{
			return std::hash<SOCKET>()(sock.pc);
		}
	};

	Socket& operator= (const Socket& pc);
	Socket& operator= (Socket&& pc);
	bool operator== (const Socket pc) const;
	bool operator!= (const Socket pc) const;
	bool operator== (const SOCKET pc) const;
	bool operator!= (const SOCKET pc) const;

	operator SOCKET&();
	operator HANDLE&();

	bool Bind(const LIB_TCHAR* port);
	Socket AcceptConnection();

	void Connect(const LIB_TCHAR* dest, const LIB_TCHAR* port, float timeout);
	void Disconnect();

	long ReadData(void* dest, DWORD nBytes);
	long SendData(const void* data, DWORD nBytes);

	void ToIp(LIB_TCHAR* ipaddr) const;
	bool IsConnected() const;

	void SetBlocking();
	void SetNonBlocking();

	//dest size needs to be at least 16
	static void GetLocalIP(LIB_TCHAR* dest);
	static void HostNameToIP(const LIB_TCHAR* host, LIB_TCHAR* dest, UINT buffSize);

private:
	SOCKET pc;
};
