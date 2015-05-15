#pragma once
#include <stdlib.h>
#include <vector>
#include <forward_list>
#include "Socket.h"
#include "Ping.h"

class TCPServ
{
public:
	struct ClientData
	{
		ClientData(TCPServ& serv, Socket pc, sfunc func, USHORT recvIndex);
		ClientData(ClientData&& clint);
		ClientData& operator=(ClientData&& data);

		Socket pc;
		sfunc func;
		PingHandler pingHandler;
		std::tstring user;
		USHORT recvIndex;
		HANDLE recvThread;
	};

	typedef void(*const customFunc)(ClientData& data);

	//sfunc is a message handler, compression is 1-9
	TCPServ(sfunc func, customFunc conFunc, customFunc disFunc, USHORT maxCon = 20, int compression = 9, float pingInterval = 30.0f, void* obj = nullptr);
	TCPServ(TCPServ&& serv);
	~TCPServ();

	TCPServ& operator=(TCPServ&& serv);

	//Allows connections to the server; should only be called once
	bool AllowConnections(const TCHAR* port);

	//Used to send data to clients
	//addr parameter functions as both the excluded address, and as a single address, 
	//depending on the value of single
	HANDLE SendClientData(char* data, DWORD nBytes, Socket addr, bool single);
	HANDLE SendClientData(char* data, DWORD nBytes, Socket* pcs, USHORT nPcs);
	HANDLE SendClientData(char* data, DWORD nBytes, std::vector<Socket>& pcs);

	//Send msg funtions used for requests, replies ect. they do not send data
	void SendMsg(Socket pc, bool single, char type, char message);
	void SendMsg(Socket* pcs, USHORT nPcs, char type, char message);
	void SendMsg(std::vector<Socket>& pcs, char type, char message);
	void SendMsg(std::tstring& user, char type, char message);

	ClientData* FindClient(const std::tstring& user);
	void Shutdown();

	//----Used in interval server code; do not use unless you know what you are doing----
	void AddClient(Socket pc);
	void RemoveClient(USHORT& pos);

	void Ping(Socket client);

	void RunConFunc(ClientData& client);
	void RunDisFunc(ClientData& client);

	//Used to wait and close the handle to a thread
	static void WaitAndCloseHandle(HANDLE& hnd);

	CRITICAL_SECTION* GetSendSect();
	//-------------------------------------------------------------------------------------

	ClientData** GetClients();
	USHORT ClientCount() const;
	void SetFunction(ClientData* client, sfunc function);
	void SetPingInterval(float interval);

	Socket& GetHost();
	bool MaxClients() const;
	void* GetObj() const;
	void WaitForRecvThread();
	int GetCompression() const;
	bool IsConnected() const;
	float GetPingInterval() const;
private:
	Socket host; //host/listener socket
	ClientData** clients; //array of clients
	USHORT nClients; //number of current connected clients
	sfunc function; //used to intialize what clients default function/msghandler is
	void* obj; //passed to function/msgHandler for oop programming
	customFunc conFunc, disFunc; //function called when connect/disconnect occurs
	CRITICAL_SECTION clientSect, sendSect; //used for synchonization
	HANDLE openCon; //wait for connections thread
	const int compression; //compression server sends packets at
	const USHORT maxCon; //max clients
	float pingInterval; //interval at which server pings inactive clients to prevent disconnect
};
