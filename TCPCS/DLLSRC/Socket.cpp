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

Socket::Socket(const LIB_TCHAR* port, bool ipv6)
{
	Bind(port, ipv6);
}


Socket::Socket()
{}

Socket::Socket(SOCKET pc)
	: 
	pc((pc != INVALID_SOCKET) ? construct<SOCKET>(pc) : nullptr),
	refCount(construct<UINT>(1))
{}

Socket::Socket(Socket&& pc)
	:
	pc(pc.pc),
	refCount(pc.refCount),
	info(std::move(pc.info))
{
	ZeroMemory(&pc, sizeof(Socket));
}

Socket::Socket(const Socket& pc)
	:
	pc(pc.pc),
	refCount(pc.refCount),
	info(pc.info)
{
	if (refCount)
		++(*refCount);
}

Socket::~Socket()
{
	if (refCount && --(*refCount) == 0)
		Disconnect();
}

Socket& Socket::operator= (const Socket& pc)
{
	if (this != &pc)
	{
		this->~Socket();
		this->pc = pc.pc;
		this->info = pc.info;
		refCount = pc.refCount;
		if (refCount)
			++(*refCount);
	}
	return *this;
}
Socket& Socket::operator= (Socket&& pc)
{
	if(this != &pc)
	{
		this->~Socket();
		this->pc = pc.pc;
		this->info = std::move(pc.info);
		refCount = pc.refCount;
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
	return (this->pc && (*this->pc == pc || *this->pc != INVALID_SOCKET));
}
bool Socket::operator!= (const SOCKET pc) const
{
	return !operator==(pc);
}

void Socket::SetSocket(SOCKET pc)
{
	if (!refCount)
	{
		refCount = construct<UINT>(1);
		this->pc = (pc != INVALID_SOCKET) ? construct<SOCKET>(pc) : nullptr;
	}
	else
		*this = Socket(pc);
}

bool Socket::Bind(const LIB_TCHAR* port, bool ipv6)
{
	ADDRINFOT info = { AI_PASSIVE, ipv6 ? AF_INET6 : AF_INET, SOCK_STREAM, IPPROTO_TCP };
	ADDRINFOT* paddr = 0;
	GetAddrInfo(NULL, port, &info, &paddr);
	ADDRINFOT& addr = *paddr;

	bool result = false;
	SOCKET pc = socket(addr.ai_family, addr.ai_socktype, addr.ai_protocol);
	if (pc != INVALID_SOCKET)
	{
		if (bind(pc, addr.ai_addr, addr.ai_addrlen) == 0)
		{
			if (listen(pc, SOMAXCONN) == 0)
			{
				SetSocket(pc);

				this->info.SetAddr(addr.ai_addr, addr.ai_addrlen);
				result = true;
			}
		}
	}

	FreeAddrInfo(paddr);

	return result;
}

Socket Socket::AcceptConnection()
{
	if(IsConnected())
	{
		int addrLen = sizeof(sockaddr_in6);
		sockaddr* addr = (sockaddr*)alloc<sockaddr_in6>();
		Socket temp(accept(*pc, addr, &addrLen));
		if (temp.IsConnected()) 
		{
			temp.info.SetAddr(addr, addrLen);
			return temp;
		}
		dealloc(addr);
	}
	return Socket();
}


//IP or HostName for dest
bool Socket::Connect(const LIB_TCHAR* dest, const LIB_TCHAR* port, bool ipv6, float timeout)
{
	int result = false;
	ADDRINFOT* addr = 0;
	ADDRINFOT info = { 0, ipv6 ? AF_INET6 : AF_INET, SOCK_STREAM, IPPROTO_TCP };

	result = GetAddrInfo(dest, port, &info, &addr);
	if(!result)
	{
		Socket temp = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);

		if (temp.IsConnected())
		{
			SOCKET tmp = *temp.pc;
			temp.SetNonBlocking();
			result = connect(tmp, addr->ai_addr, addr->ai_addrlen);
			if(result == SOCKET_ERROR)  // connect fails right away
			{
				FD_SET fds;
				FD_ZERO(&fds);
				FD_SET(tmp, &fds);
				u_long sec = (long)floor(timeout);
				TIMEVAL tv = { sec, long((timeout - sec) * 100000.f) };
				if(select(0, nullptr, &fds, nullptr, &tv) <= 0)
					temp.~Socket();  // timed out
				else
					temp.SetBlocking();
			}
			else
			{
				temp.SetBlocking();
			}
		}
		if (temp.IsConnected())
		{
			*this = std::move(temp);

			this->info.SetAddr(addr->ai_addr, addr->ai_addrlen);
		}

		FreeAddrInfo(addr);
	}

	return IsConnected();
}

void Socket::Disconnect()
{
	if (IsConnected())
	{
		shutdown(*pc, SD_BOTH);
		closesocket(*pc);
		destruct(pc);
		info.Cleanup();
	}
	destruct(refCount);
}


long Socket::ReadData(void* dest, DWORD nBytes)
{
#if NTDDI_VERSION >= NTDDI_VISTA
	return recv(*pc, (char*)dest, nBytes, MSG_WAITALL);
#else
	long received = 0, temp = SOCKET_ERROR;
	do
	{
		received += temp = recv(*pc, ((char*)dest) + received, nBytes - received, 0);
		if ((temp == 0) || (temp == SOCKET_ERROR))
			return temp;
	} while (received != nBytes);
	return received;
#endif
}

long Socket::SendData(const void* data, DWORD nBytes)
{
	long sent = 0, temp = SOCKET_ERROR;
	do
	{
		sent += temp = send(*pc, ((char*)data) + sent, nBytes - sent, 0);
		if (temp == SOCKET_ERROR)
			return temp;
	} while(sent != nBytes);
	return sent;
}

bool Socket::IsConnected() const
{
	return pc && *pc != INVALID_SOCKET;
}


bool Socket::SetBlocking()
{
	u_long nbio = 0;
	return (ioctlsocket(*pc, FIONBIO, &nbio) != SOCKET_ERROR);
}

bool Socket::SetNonBlocking()
{
	u_long nbio = 1;
	return (ioctlsocket(*pc, FIONBIO, &nbio) != SOCKET_ERROR);
}

const SocketInfo& Socket::GetInfo()
{
	return info;
}

bool Socket::GetLocalIP(LIB_TCHAR* dest, bool ipv6)
{
	DWORD buffSize = INET6_ADDRSTRLEN;
	LIB_TCHAR buffer[128] = {};
	ADDRINFOT info = { 0, ipv6 ? AF_INET6 : AF_INET }, *pa = nullptr;
	GetHostName(buffer, 128);
	bool res = (GetAddrInfo(buffer, NULL, &info, &pa) == 0);

	if (res)
		res &= WSAAddressToString(pa->ai_addr, pa->ai_addrlen, NULL, dest, &buffSize);

	FreeAddrInfo(pa);

	return  res;
}

bool Socket::HostNameToIP(const LIB_TCHAR* host, LIB_TCHAR* dest, bool ipv6)
{
	DWORD buffSize = INET6_ADDRSTRLEN;
	ADDRINFOT info = { 0, ipv6 ? AF_INET6 : AF_INET }, *pa = nullptr;
	bool res = (GetAddrInfo(host, NULL, &info, &pa) == 0);

	if (res)
		res &= WSAAddressToString(pa->ai_addr, pa->ai_addrlen, NULL, dest, &buffSize);

	FreeAddrInfo(pa);

	return res;
}

UINT Socket::GetRefCount() const
{
	return refCount ? *refCount : 0;
}
