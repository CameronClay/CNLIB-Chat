#pragma once
#include "Typedefs.h"
#include "Socket.h"

class CAMSNETLIB SocketListen
{
public:
	SocketListen();
	SocketListen(SocketListen&& socket);
	~SocketListen();

	bool Bind(const LIB_TCHAR* port, bool ipv6);
	void StartThread(class TCPServ& serv);
	void Destroy();

	Socket& GetHost();
private:
	Socket host;
};
