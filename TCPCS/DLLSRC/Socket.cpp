#include "StdAfx.h"
#include "CNLIB/Socket.h"
#include "CNLIB/HeapAlloc.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib") //for AcceptEx

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
	pc(pc)
{}

Socket::Socket(Socket&& pc)
	:
	pc(pc.pc),
	info(std::move(pc.info))
{
	ZeroMemory(&pc, sizeof(Socket));
}

Socket::Socket(const Socket& pc)
	:
	pc(pc.pc),
	info(pc.info)
{}

Socket& Socket::operator= (const Socket& pc)
{
	if (this != &pc)
	{
		Disconnect();
		this->pc = pc.pc;
		this->info = pc.info;
	}
	return *this;
}
Socket& Socket::operator= (Socket&& pc)
{
	if (this != &pc)
	{
		Disconnect();
		this->pc = pc.pc;
		this->info = std::move(pc.info);
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
	return this->pc == pc;
}
bool Socket::operator!= (const SOCKET pc) const
{
	return !operator==(pc);
}

void Socket::SetSocket(SOCKET pc)
{
	if (!IsConnected())
		this->pc = pc;
	else
		*this = Socket(pc);
}

SOCKET Socket::GetSocket() const
{
	return pc;
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

				sockaddr* temp = (sockaddr*)alloc<char>(addr.ai_addrlen);
				memcpy(temp, addr.ai_addr, addr.ai_addrlen);
				this->info.SetAddr(temp, true);
				result = true;
			}
		}
	}

	FreeAddrInfo(paddr);

	return result;
}

Socket Socket::AcceptConnection()
{
	Socket temp;
	if (IsConnected())
	{
		int addrLen = sizeof(sockaddr_in6);
		sockaddr* addr = (sockaddr*)alloc<sockaddr_in6>();
		temp = accept(pc, addr, &addrLen);
		if (temp.IsConnected())
		{
			temp.info.SetAddr(addr, true);
		}
		dealloc(addr);
	}
	return temp;
}

//IP or HostName for dest
bool Socket::Connect(const LIB_TCHAR* dest, const LIB_TCHAR* port, bool ipv6, int tcpSendSize, int tcpRecvSize, bool noDelay, float timeout)
{
	int result = false;
	ADDRINFOT* addr = 0;
	ADDRINFOT info = { 0, ipv6 ? AF_INET6 : AF_INET, SOCK_STREAM, IPPROTO_TCP };

	result = GetAddrInfo(dest, port, &info, &addr);
	if (!result)
	{
		Socket temp = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);

		temp.SetTCPSendStack(tcpSendSize);
		temp.SetTCPRecvStack(tcpRecvSize);
		temp.SetNoDelay(noDelay);
		temp.SetNonBlocking();

		result = connect(temp.pc, addr->ai_addr, addr->ai_addrlen);
		if (result == SOCKET_ERROR)  // connect fails right away
		{
			FD_SET fds;
			FD_ZERO(&fds);
			FD_SET(temp.pc, &fds);
			const u_long sec = (long)floor(timeout);
			TIMEVAL tv = { sec, long((timeout - sec) * 100000.f) };
			if (select(0, nullptr, &fds, nullptr, &tv) <= 0)
			{
				closesocket(temp.pc);
				FreeAddrInfo(addr);
				return false; // probly timed out
			}
		}

		temp.SetBlocking();
		*this = std::move(temp);

		sockaddr* buffer = (sockaddr*)alloc<char>(addr->ai_addrlen);
		memcpy(buffer, addr->ai_addr, addr->ai_addrlen);
		this->info.SetAddr(buffer, true);
		FreeAddrInfo(addr);
	}
	return IsConnected();
}


void Socket::Shutdown()
{
	shutdown(pc, SD_BOTH);
}

void Socket::Disconnect()
{
	if (IsConnected())
	{
		info.Cleanup();
		Shutdown();
		closesocket(pc);
		pc = INVALID_SOCKET;
	}
}


long Socket::RecvData(void* dest, DWORD nBytes)
{
#if NTDDI_VERSION >= NTDDI_VISTA
	return recv(pc, (char*)dest, nBytes, MSG_WAITALL);
#else
	long received = 0, temp = SOCKET_ERROR;
	do
	{
		received += temp = recv(pc, ((char*)dest) + received, nBytes - received, 0);
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
		sent += temp = send(pc, ((char*)data) + sent, nBytes - sent, 0);
		if (temp == SOCKET_ERROR)
			return temp;
	} while (sent != nBytes);
	return sent;
}

bool Socket::AcceptOl(SOCKET acceptSocket, void* infoBuffer, DWORD localAddrLen, DWORD remoteAddrLen, OVERLAPPED* ol)
{
	return AcceptEx(pc, acceptSocket, infoBuffer, 0, localAddrLen, remoteAddrLen, NULL, ol);
}

bool Socket::SetAcceptExContext(SOCKET accept, SOCKET listen)
{
	return (setsockopt(accept, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (const char*)&listen, sizeof(SOCKET)) == 0);
}

long Socket::RecvDataOl(WSABUF* buffer, OVERLAPPED* ol, DWORD flags, UINT bufferCount, LPWSAOVERLAPPED_COMPLETION_ROUTINE cr)
{
	return WSARecv(pc, buffer, bufferCount, NULL, &flags, ol, cr);
}

long Socket::SendDataOl(WSABUF* buffer, OVERLAPPED* ol, DWORD flags, UINT bufferCount, LPWSAOVERLAPPED_COMPLETION_ROUTINE cr)
{
	return WSASend(pc, buffer, bufferCount, NULL, flags, ol, cr);
}

bool Socket::IsConnected() const
{
	return pc != INVALID_SOCKET;
}

bool Socket::SetTCPRecvStack(int size)
{
	return (setsockopt(pc, SOL_SOCKET, SO_RCVBUF, (const char*)&size, sizeof(int)) == 0);
}
bool Socket::SetTCPSendStack(int size)
{
	return (setsockopt(pc, SOL_SOCKET, SO_SNDBUF, (const char*)&size, sizeof(int)) == 0);
}
bool Socket::SetNoDelay(bool b)
{
	BOOL temp = b;
	return (setsockopt(pc, IPPROTO_TCP, TCP_NODELAY, (const char*)&temp, sizeof(BOOL)) == 0);
}

bool Socket::GetTCPRecvStack(int& size)
{
	int paramSize = sizeof(int);
	return (getsockopt(pc, SOL_SOCKET, SO_RCVBUF, (char*)&size, &paramSize) == 0);
}
bool Socket::GetTCPSendStack(int& size)
{
	int paramSize = sizeof(int);
	return (getsockopt(pc, SOL_SOCKET, SO_SNDBUF, (char*)&size, &paramSize) == 0);
}
bool Socket::GetTCPNoDelay(bool& b)
{
	int paramSize = sizeof(BOOL);
	return (getsockopt(pc, IPPROTO_TCP, TCP_NODELAY, (char*)&b, &paramSize) == 0);
}

bool Socket::SetBlocking()
{
	u_long nbio = 0;
	return (ioctlsocket(pc, FIONBIO, &nbio) != SOCKET_ERROR);
}

bool Socket::SetNonBlocking()
{
	u_long nbio = 1;
	return (ioctlsocket(pc, FIONBIO, &nbio) != SOCKET_ERROR);
}

const SocketInfo& Socket::GetInfo()
{
	return info;
}

void Socket::SetAddrInfo(sockaddr* addr, bool cleanup)
{
	info.SetAddr(addr, cleanup);
}

std::tstring Socket::GetLocalIP(bool ipv6)
{
	DWORD buffSize = INET6_ADDRSTRLEN;
	LIB_TCHAR buffer[128] = {};
	bool res = (GetHostName(buffer, 128) == 0);
	if (res)
	{
		std::tstring temp;
		ADDRINFOT info = { 0, ipv6 ? AF_INET6 : AF_INET }, *pa = nullptr;
		res &= (GetAddrInfo(buffer, NULL, &info, &pa) == 0);

		if (res)
			temp = SocketInfo::FormatIP(pa->ai_addr);

		FreeAddrInfo(pa);

		return temp;
	}

	return std::tstring();
}

std::tstring Socket::HostNameToIP(const LIB_TCHAR* host, bool ipv6)
{
	std::tstring temp;
	DWORD buffSize = INET6_ADDRSTRLEN;
	ADDRINFOT info = { 0, ipv6 ? AF_INET6 : AF_INET }, *pa = nullptr;
	bool res = (GetAddrInfo(host, NULL, &info, &pa) == 0);

	if (res)
		temp = SocketInfo::FormatIP(pa->ai_addr);

	FreeAddrInfo(pa);

	return temp;
}

int Socket::GetHostName(LIB_TCHAR* dest, DWORD nameLen)
{
	int res = 0;
#ifdef UNICODE
	char host[256];
	res = gethostname(host, 256);
	MultiByteToWideChar(CP_ACP, 0, host, 256, dest, nameLen);
#else
	res = gethostname(dest, nameLen);
#endif
	return res;
}

char* Socket::Inet_ntot(in_addr inaddr, LIB_TCHAR* dest)
{
	char* addr = nullptr;
#ifdef UNICODE
	addr = inet_ntoa(inaddr);
	MultiByteToWideChar(CP_ACP, 0, addr, strlen(addr), dest, strlen(addr));
#else
	addr = inet_ntoa(inaddr);
	memcpy(dest, addr, sizeof(char) * strlen(addr));
#endif
	return addr;
}
void Socket::GetAcceptExAddrs(void* buffer, DWORD localAddrLen, DWORD remoteAddrLen, sockaddr** local, int* localLen, sockaddr** remote, int* remoteLen)
{
	GetAcceptExSockaddrs(buffer, 0, localAddrLen, remoteAddrLen, local, localLen, remote, remoteLen);
}
