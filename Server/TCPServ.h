#pragma once
#include <stdlib.h>
#include <vector>
#include <forward_list>
#include "Socket.h"
#include "Ping.h"

#define INACTIVETIME 30.0f
#define PINGTIME 2.0f

class TCPServ
{
public:
	struct ClientData
	{
		ClientData(Socket pc, HANDLE recvThread, sfunc func)
			:
			pc(pc),
			recvThread(recvThread),
			func(func),
			pingHandler()
		{}

		ClientData(ClientData&& clint)
			:
			pc(clint.pc),
			recvThread(clint.recvThread),
			func(clint.func),
			user(clint.user),
			pingHandler(clint.pingHandler)
		{
			ZeroMemory(&clint, sizeof(ClientData));
		}

		Socket pc;
		HANDLE recvThread;
		sfunc func;
		std::tstring user;
		PingHandler pingHandler;	
	};

	typedef void(*const customFunc)(ClientData& data);

	//compression 1-9
	TCPServ(USHORT maxCon, sfunc func, void* obj, int compression, customFunc conFunc, customFunc disFunc);
	TCPServ(TCPServ&& serv);
	~TCPServ();

	void AllowConnections(const TCHAR* port);//Starts receiving as well
	//addr functions as excluded address or single address based on single value
	HANDLE SendClientData(char* data, DWORD nBytes, Socket addr, bool single);
	HANDLE SendClientData(char* data, DWORD nBytes, std::vector<Socket>& pcs);

	void AddClient(Socket pc);
	void RemoveClient(USHORT& pos);

	void Shutdown();

	static void WaitAndCloseHandle(HANDLE& hnd);

	void Ping(Socket& client);

	void RunConFunc(ClientData& client);
	void RunDisFunc(ClientData& client);

	std::vector<ClientData>& GetClients();
	std::vector<USHORT*>& GetRecvIndices();
	void SetFunction(USHORT index, sfunc function);

	Socket& GetHost();
	bool MaxClients() const;
	void* GetObj() const;
	void WaitForRecvThread();
	int GetCompression() const;
private:
	Socket host;
	HANDLE openCon;
	std::vector<ClientData> clients;
	std::vector<USHORT*> recvIndex;
	sfunc function;
	void* obj;
	int compression;
	customFunc conFunc, disFunc;
	USHORT maxCon;
};
