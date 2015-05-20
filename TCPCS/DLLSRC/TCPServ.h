#pragma once
#include "TCPServInterface.h"
#include "HeapAlloc.h"

class TCPServ : public TCPServInterface
{
public:
	//sfunc is a message handler, compression is 1-9
	TCPServ(sfunc func, customFunc conFunc, customFunc disFunc, USHORT maxCon = 20, int compression = 9, float pingInterval = 30.0f, void* obj = nullptr);
	TCPServ(TCPServ&& serv);
	~TCPServ();

	TCPServ& operator=(TCPServ&& serv);

	//Allows connections to the server; should only be called once
	bool AllowConnections(const LIB_TCHAR* port);

	//Used to send data to clients
	//addr parameter functions as both the excluded address, and as a single address, 
	//depending on the value of single
	HANDLE SendClientData(const char* data, DWORD nBytes, Socket addr, bool single);
	HANDLE SendClientData(const char* data, DWORD nBytes, Socket* pcs, USHORT nPcs);
	HANDLE SendClientData(const char* data, DWORD nBytes, std::vector<Socket>& pcs);

	//Send msg funtions used for requests, replies ect. they do not send data
	void SendMsg(Socket pc, bool single, char type, char message);
	void SendMsg(Socket* pcs, USHORT nPcs, char type, char message);
	void SendMsg(std::vector<Socket>& pcs, char type, char message);
	void SendMsg(const std::tstring& user, char type, char message);

	ClientData* FindClient(const std::tstring& user) const;

	void Shutdown();

	void AddClient(Socket pc);
	void RemoveClient(USHORT& pos);

	void Ping(Socket client);

	void RunConFunc(ClientData* client);
	void RunDisFunc(ClientData* client);

	CRITICAL_SECTION* GetSendSect();

	ClientData** GetClients() const;
	USHORT ClientCount() const;
	void SetPingInterval(float interval);

	Socket& GetHost();
	bool MaxClients() const;
	void* GetObj() const;
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

