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

		~ClientData(){}

		ClientData& operator=(ClientData&& data)
		{
			pc = std::move(data.pc);
			recvThread = data.recvThread;
			func = data.func;
			user = std::move(data.user);
			pingHandler = std::move(data.pingHandler);

			ZeroMemory(&data, sizeof(ClientData));
			return *this;
		}
		
		Socket pc;
		HANDLE recvThread;
		sfunc func;
		std::tstring user;
		PingHandler pingHandler;	
	};

	typedef void(*const customFunc)(ClientData& data);

	//sfunc is a message handler, compression is 1-9
	TCPServ(USHORT maxCon, sfunc func, void* obj, int compression, customFunc conFunc, customFunc disFunc);
	TCPServ(TCPServ&& serv);
	~TCPServ();

	TCPServ& operator=(TCPServ&& serv);

	void AllowConnections(const TCHAR* port);//Starts receiving as well

	//addr parameter functions as both the excluded address, and as a single address, depending on the value of single
	HANDLE SendClientData(char* data, DWORD nBytes, Socket addr, bool single);
	HANDLE SendClientData(char* data, DWORD nBytes, std::vector<Socket>& pcs);

	//send msg funtions used for requests, replies ect. they do not send data
	void SendMsg(Socket pc, bool single, char type, char message);
	void SendMsg(std::vector<Socket>& pcs, char type, char message);
	void SendMsg(std::tstring& user, char type, char message);

	void AddClient(Socket pc);
	void RemoveClient(USHORT& pos);
	ClientData* FindClient(std::tstring &user);
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
