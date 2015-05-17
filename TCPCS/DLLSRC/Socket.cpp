#include "Socket.h"

#include <ws2tcpip.h>
#include <Mstcpip.h>
#include <functional>

#include "HeapAlloc.h"
#include "File.h"

int InitializeNetworking()
{
	WSADATA wsad{};
	return WSAStartup(MAKEWORD(2, 2), &wsad);
}

int CleanupNetworking()
{
	return WSACleanup();
}

Socket::Socket(const TCHAR* port)
{
	Bind(port);
}


Socket::Socket()
	:
	pc(INVALID_SOCKET)
{}

Socket::Socket(SOCKET pc)
	:
	pc(pc)
{}

Socket::Socket(Socket&& sock)
	:
	pc(sock.pc)
{}

Socket::~Socket(){}


Socket& Socket::operator= (Socket& pc)
{
	this->pc = pc.pc;
	return *this;
}
Socket& Socket::operator= (Socket&& pc)
{
	if(this != &pc)
	{
		this->pc = pc.pc;
		ZeroMemory(&pc, sizeof(Socket));
	}
	return *this;
}

bool Socket::operator== (const Socket pc) const
{
	return pc.pc == this->pc;
}
bool Socket::operator!= (const Socket pc) const
{
	return pc.pc != this->pc;
}
bool Socket::operator== (const SOCKET pc) const
{
	return pc == this->pc;
}
bool Socket::operator!= (const SOCKET pc) const
{
	return pc != this->pc;
}

Socket::operator SOCKET&()
{
	return pc;
}

Socket::operator HANDLE&()
{
	return (HANDLE&)pc;
}

void Socket::Bind(const TCHAR* port)
{
	ADDRINFOT info = { AI_PASSIVE, AF_INET, SOCK_STREAM, IPPROTO_TCP };
	ADDRINFOT* addr = 0;
	GetAddrInfo(NULL, port, &info, &addr);
	pc = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	bind(pc, addr->ai_addr, addr->ai_addrlen);
	listen(pc, SOMAXCONN);
	FreeAddrInfo(addr);
}

Socket Socket::AcceptConnection()
{
	if(IsConnected())
	{
		SOCKET temp = accept(pc, NULL, NULL);
		if(temp != INVALID_SOCKET)
		{
			return Socket(temp);
		}
	}
	return Socket();
}


//IP or HostName for dest
void Socket::Connect(const TCHAR* dest, const TCHAR* port, float timeout)
{
	SOCKET sock = INVALID_SOCKET;
	int result = false;
	ADDRINFOT* addr = 0;
	ADDRINFOT info = { 0, AF_INET, SOCK_STREAM, IPPROTO_TCP };
	//info.ai_flags = AI_NUMERICHOST;
	result = GetAddrInfo(dest, port, &info, &addr);
	if(!result)
	{
		pc = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
		if(pc != INVALID_SOCKET)
		{
			SetNonBlocking();
			result = connect(pc, addr->ai_addr, addr->ai_addrlen);
			if(result == SOCKET_ERROR)
			{
				FD_SET fds;
				FD_ZERO(&fds);
				FD_SET(pc, &fds);
				u_long sec = (long)floor(timeout);
				TIMEVAL tv = { sec, long((timeout - sec) * 100000.f) };
				if(select(0, nullptr, &fds, nullptr, &tv) <= 0)
				{
					closesocket(pc);
					pc = INVALID_SOCKET;
				}
				else
				{
					SetBlocking();
				}
			}
			else
			{
				SetBlocking();
			}
		}
		FreeAddrInfo(addr);
	}
}

void Socket::Disconnect()
{
	if(IsConnected())
	{
		shutdown(pc, SD_BOTH);
		closesocket(pc);
		pc = INVALID_SOCKET;
	}
}


long Socket::ReadData(void* dest, DWORD nBytes)
{
#if NTDDI_VERSION >= NTDDI_VISTA
	return recv(pc, (char*)dest, nBytes, MSG_WAITALL);
#else
	long received = 0;
	do
	{
		received += recv(pc, &(((char*)dest)[received]), nBytes - received, 0);
	} while(received != nBytes && received > 0);
	return received;
#endif
}

long Socket::SendData(const void* data, DWORD nBytes)
{
	long sent = 0;
	do
	{
		sent += send(pc, &(((char*)data)[sent]), nBytes - sent, 0);
	} while(sent != nBytes && sent > 0);
	return sent;
}


void Socket::ToIp(TCHAR* ipaddr) const
{
	sockaddr_in saddr = {};
	int len = sizeof(saddr);
	getpeername(pc, (sockaddr*)&saddr, &len);
#if NTDDI_VERSION >= NTDDI_VISTA
	InetNtop(saddr.sin_family, &saddr.sin_addr, ipaddr, INET_ADDRSTRLEN);
#else
	Inet_ntot(saddr.sin_addr, ipaddr);
#endif
}

bool Socket::IsConnected() const
{
	return pc != INVALID_SOCKET;
}


void Socket::GetLocalIP(TCHAR* dest)
{
	TCHAR buffer[255] = {};
	ADDRINFOT info = { 0, AF_INET }, *pa = nullptr;
	GetHostName(buffer, 255);
	GetAddrInfo(buffer, NULL, &info, &pa);

#if NTDDI_VERSION >= NTDDI_VISTA
	RtlIpv4AddressToString(&((sockaddr_in*)pa->ai_addr)->sin_addr, dest);
#else
	DWORD buffSize = INET_ADDRSTRLEN;
	WSAAddressToString(pa->ai_addr, pa->ai_addrlen, NULL, dest, &buffSize);
#endif

	FreeAddrInfo(pa);
}

void Socket::HostNameToIP(const TCHAR* host, TCHAR* dest, UINT buffSize)
{
	ADDRINFOT info = { 0, AF_INET }, *p = nullptr;
	GetAddrInfo(host, NULL, &info, &p);

#if NTDDI_VERSION >= NTDDI_VISTA
	InetNtop(p->ai_family, p->ai_addr, dest, buffSize);
#else
	Inet_ntot(((sockaddr_in*)p->ai_addr)->sin_addr, dest);
#endif
	FreeAddrInfo(p);
}

void Socket::SetBlocking()
{
	u_long nbio = 0;
	ioctlsocket(pc, FIONBIO, &nbio);
}

void Socket::SetNonBlocking()
{
	u_long nbio = 1;
	ioctlsocket(pc, FIONBIO, &nbio);
}

