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

	bool AllowConnections(const TCHAR* port);//Starts receiving as well

	// addr parameter functions as both the excluded address, and as a single address, 
	// depending on the value of single
	HANDLE SendClientData(char* data, DWORD nBytes, Socket addr, bool single);
	HANDLE SendClientData(char* data, DWORD nBytes, Socket* pcs, USHORT nPcs);
	HANDLE SendClientData(char* data, DWORD nBytes, std::vector<Socket>& pcs);

	// send msg funtions used for requests, replies ect. they do not send data
	void SendMsg(Socket pc, bool single, char type, char message);
	void SendMsg(Socket* pcs, USHORT nPcs, char type, char message);
	void SendMsg(std::vector<Socket>& pcs, char type, char message);
	void SendMsg(std::tstring& user, char type, char message);

	void AddClient(Socket pc);
	void RemoveClient(USHORT& pos);
	ClientData* FindClient(const std::tstring& user);
	void Shutdown();

	static void WaitAndCloseHandle(HANDLE& hnd);

	void Ping(Socket client);

	void RunConFunc(ClientData& client);
	void RunDisFunc(ClientData& client);

	ClientData**& GetClients();
	USHORT ClientCount() const;
	void SetFunction(ClientData* client, sfunc function);
	void SetPingInterval(float interval);

	CRITICAL_SECTION* GetSendSect();

	Socket& GetHost();
	bool MaxClients() const;
	void* GetObj() const;
	void WaitForRecvThread();
	int GetCompression() const;
	bool IsConnected() const;
	float GetPingInterval() const;
private:
	Socket host;
	ClientData** clients;
	USHORT nClients;
	sfunc function;
	void* obj;
	customFunc conFunc, disFunc;
	CRITICAL_SECTION clientSect, sendSect;
	HANDLE openCon;
	const int compression;
	const USHORT maxCon;
	float pingInterval;
};
