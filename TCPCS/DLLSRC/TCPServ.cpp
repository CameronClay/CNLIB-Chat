#include "StdAfx.h"
#include "TCPServ.h"
#include "CNLIB/BufSize.h"
#include "CNLIB/File.h"
#include "CNLIB/Messages.h"
//#include "CNLIB/MsgStream.h"

TCPServInterface* CreateServer(sfunc msgHandler, ConFunc conFunc, DisconFunc disFunc, DWORD nThreads, DWORD nConcThreads, UINT maxDataSize, UINT maxCon, int compression, int compressionCO, float keepAliveInterval, bool noDelay, void* obj)
{
	return construct<TCPServ>(msgHandler, conFunc, disFunc, nThreads, nConcThreads, maxDataSize, maxCon, compression, compressionCO, keepAliveInterval, noDelay, obj);
}
void DestroyServer(TCPServInterface*& server)
{
	destruct((TCPServ*&)server);
}


ClientAccess::ClientAccess(ClientData** clients)
	:
	clients((const ClientData**)clients)
{}
ClientData* ClientAccess::operator+(UINT amount)
{
	return *(ClientData**)((ClientDataEx**)clients + amount);
}
ClientData* ClientAccess::operator-(UINT amount)
{
	return *(ClientData**)((ClientDataEx**)clients - amount);
}
ClientData* ClientAccess::operator[](UINT index)
{
	return *(ClientData**)((ClientDataEx**)clients + index);
}

ClientData::ClientData(Socket pc, sfunc func)
	:
	pc(pc),
	func(func),
	obj(nullptr)
{}
ClientData::ClientData(ClientData&& clint)
	:
	pc(std::move(clint.pc)),
	func(clint.func),
	user(std::move(clint.user)),
	obj(clint.obj)
{
	ZeroMemory(&clint, sizeof(ClientData));
}
ClientData& ClientData::operator=(ClientData&& data)
{
	if(this != &data)
	{
		pc = std::move(data.pc);
		func = data.func;
		user = std::move(data.user);
		obj = data.obj;

		ZeroMemory(&data, sizeof(ClientData));
	}
	return *this;
}


ClientDataEx::ClientDataEx(TCPServ& serv, Socket pc, sfunc func, UINT arrayIndex)
	:
	ClientData(pc, func),
	serv(serv),
	ol(OpType::recv),
	arrayIndex(arrayIndex),
	state(running)
{
	MemPool<HeapAllocator>& pool = serv.GetRecvBuffPool();
	const UINT maxDataSize = serv.MaxDataSize();
	char* temp = pool.alloc<char>();
	buff.Initialize(maxDataSize, temp, temp);
}
ClientDataEx::ClientDataEx(ClientDataEx&& clint)
	:
	ClientData(std::forward<ClientData>(*this)),
	serv(clint.serv),
	buff(clint.buff),
	ol(clint.ol),
	arrayIndex(clint.arrayIndex),
	state(clint.state)
{
	ZeroMemory(&clint, sizeof(ClientDataEx));
}
ClientDataEx& TCPServ::ClientDataEx::operator=(ClientDataEx&& data)
{
	if (this != &data)
	{
		__super::operator=(std::forward<ClientData>(data));
		buff = data.buff;
		arrayIndex = data.arrayIndex;
		ol = data.ol;
		state = data.state;

		memset(&data, 0, sizeof(ClientDataEx));
	}

	return *this;
}
ClientDataEx::~ClientDataEx()
{
	serv.GetRecvBuffPool().dealloc(buff.head);
}


HostSocket::HostSocket(TCPServ& serv)
	:
	serv(serv),
	buffer(),
	ol(OpType::accept)
{}
HostSocket::HostSocket(HostSocket&& host)
	:
	serv(host.serv),
	listen(std::move(host.listen)),
	accept(host.accept),
	buffer(host.buffer),
	ol(host.ol)
{
	memset(&host, 0, sizeof(HostSocket));
}
HostSocket& HostSocket::operator=(HostSocket&& data)
{
	if (this != &data)
	{
		listen = std::move(data.listen);
		accept = data.accept;
		buffer = data.buffer;
		ol = data.ol;

		memset(&data, 0, sizeof(HostSocket));
	}
	return *this;
}
HostSocket::~HostSocket()
{
	dealloc(buffer); //only deallocs if was allocated
}

void HostSocket::SetAcceptSocket()
{
	accept = socket(AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP);
}
void HostSocket::CloseAcceptSocket()
{
	closesocket(accept);
}


DWORD CALLBACK IOCPThread(LPVOID info)
{
	IOCP& iocp = *(IOCP*)info;
	DWORD bytesTrans = 0;
	OverlappedExt* ol = nullptr;
	ULONG_PTR key = NULL;
	
	bool res = false;
	do
	{
		res = GetQueuedCompletionStatus(iocp.GetHandle(), &bytesTrans, &key, (OVERLAPPED**)&ol, INFINITE);
		if (res)
		{
			if (!ol)
				break;
			switch (ol->opType)
			{
				case OpType::recv:
				{
					ClientDataEx& cd = *(ClientDataEx*)key;
					if (bytesTrans == 0)
					{
						cd.serv.RemoveClient(&cd, cd.state != ClientDataEx::closing);
						continue;
					}
					cd.serv.RecvDataCR(bytesTrans, cd, ol);
				}
				break;
				case OpType::send:
				case OpType::sendmsg:
				{
					ClientDataEx& cd = *(ClientDataEx*)key;
					cd.serv.SendDataCR(cd, (OverlappedSend*)ol);
				}
				break;
				case OpType::accept:
				{
					HostSocket& hostSocket = *(HostSocket*)key;
					hostSocket.serv.AcceptConCR(hostSocket, ol);
				}
				break;
			}
		}
		else
		{
			if (ol && key)
			{
				if (ol->opType == OpType::send || ol->opType == OpType::sendmsg)
				{
					ClientDataEx& cd = *(ClientDataEx*)key;
					cd.serv.SendDataCR(cd, (OverlappedSend*)ol);
				}
				else if (ol->opType == OpType::recv)
				{
					ClientDataEx* cd = (ClientDataEx*)key;
					cd->serv.RemoveClient(cd, cd->state != ClientDataEx::closing); //closing flag only set when you initiated close
				}
				else
				{
					HostSocket& hostSocket = *(HostSocket*)key;
					hostSocket.serv.CleanupAcceptEx(hostSocket);
				}
			}
			//else
			//{
			//	//Error
			//}
		}
	} while (res);

	return 0;
}


char* TCPServ::GetSendBuffer()
{
	return sendDataPool.alloc<char>() + sizeof(DWORD64);
}

WSABufExt TCPServ::CreateSendBuffer(DWORD nBytesDecomp, char* buffer, OpType opType, CompressionType compType)
{
	DWORD nBytesComp = 0, nBytesSend = nBytesDecomp;
	char* dest = buffer;

	if (opType == OpType::sendmsg)
	{
		char* temp = buffer;
		dest = buffer = sendMsgPool.alloc<char>();
		*(int*)(buffer + sizeof(DWORD64)) = *(int*)temp;
	}
	else
	{
		buffer -= sizeof(DWORD64);
		if (nBytesDecomp > maxDataSize)
		{
			sendDataPool.destruct(buffer);
			return {};
		}
	}

	if (compType == BESTFIT)
	{
		if (nBytesDecomp >= compressionCO)
			compType = SETCOMPRESSION;
		else
			compType = NOCOMPRESSION;
	}

	if (compType == SETCOMPRESSION)
	{
		nBytesComp = FileMisc::Compress((BYTE*)(buffer + maxDataSize + sizeof(DWORD64)), maxCompSize, (const BYTE*)(buffer + sizeof(DWORD64)), nBytesDecomp, compression);
		if (nBytesComp < nBytesDecomp)
		{
			nBytesSend = nBytesComp;
			dest = buffer + maxDataSize;
		}
	}

	*(DWORD64*)(dest) = ((DWORD64)nBytesDecomp) << 32 | nBytesComp;

	
	WSABufExt buf;
	buf.Initialize(nBytesSend, dest, buffer);
	return buf;
}
void TCPServ::FreeSendBuffer(WSABufExt buff, OpType opType)
{
	if (opType == OpType::send)
		sendDataPool.dealloc(buff.head);
	else
		sendMsgPool.dealloc(buff.head);
}
void TCPServ::FreeSendOverlapped(OverlappedSend* ol)
{
	if (sendOlPoolAll.InPool(ol->head))
		sendOlPoolAll.dealloc(ol->head);
	else
		sendOlPoolSingle.dealloc(ol->head);
}

void TCPServ::SendClientData(const char* data, DWORD nBytes, Socket exAddr, bool single, CompressionType compType)
{
	SendClientData(data, nBytes, exAddr, single, OpType::send, compType);
}
void TCPServ::SendClientData(const char* data, DWORD nBytes, Socket* pcs, UINT nPcs, CompressionType compType)
{
	SendClientData(data, nBytes, pcs, nPcs, OpType::send, compType);
}
void TCPServ::SendClientData(const char* data, DWORD nBytes, std::vector<Socket>& pcs, CompressionType compType)
{
	SendClientData(data, nBytes, pcs.data(), (USHORT)pcs.size(), compType);
}

void TCPServ::SendClientData(const char* data, DWORD nBytes, Socket exAddr, bool single, OpType opType, CompressionType compType)
{
	long res = 0;

	if (single)
	{
		WSABufExt sendBuff = CreateSendBuffer(nBytes, (char*)data, opType, compType);
		if (!sendBuff.head)
			return;

		OverlappedSend* ol = sendOlPoolSingle.construct<OverlappedSend>(1);
		ol->InitInstance(opType, sendBuff, ol);
		++opCounter;

		res = exAddr.SendDataOl(&ol->sendBuff, ol);
		if ((res == SOCKET_ERROR) && (WSAGetLastError() != WSA_IO_PENDING))
		{
			--opCounter;
			FreeSendBuffer(sendBuff, opType);
			sendOlPoolSingle.destruct(ol);
		}
	}
	else
	{
		WSABufExt sendBuff = CreateSendBuffer(nBytes, (char*)data, opType, compType);
		if (!sendBuff.head)
			return;

		OverlappedSend* ol = sendOlPoolAll.construct<OverlappedSend>(nClients);
		opCounter += nClients;
		for (UINT i = 0; i < nClients; i++)
		{
			Socket& pc = clients[i]->pc;
			ol[i].InitInstance(opType, sendBuff, ol);
			if (pc != exAddr)
				res = pc.SendDataOl(&ol[i].sendBuff, ol);

			if (pc == exAddr || ((res == SOCKET_ERROR) && (WSAGetLastError() != WSA_IO_PENDING)))
			{
				--opCounter;
				if (ol->DecrementRefCount())
				{
					sendOlPoolAll.destruct(ol);
					FreeSendBuffer(sendBuff, opType);
				}
			}
		}
	}
}
void TCPServ::SendClientData(const char* data, DWORD nBytes, Socket* pcs, UINT nPcs, OpType opType, CompressionType compType)
{
	long res = 0;

	if (nPcs == 1)
	{
		Socket& pc = *pcs;
		if (pc.IsConnected())
		{
			WSABufExt sendBuff = CreateSendBuffer(nBytes, (char*)data, opType, compType);
			if (!sendBuff.head)
				return;

			OverlappedSend* ol = sendOlPoolSingle.construct<OverlappedSend>(1);
			ol->InitInstance(opType, sendBuff, ol);
			++opCounter;

			res = pc.SendDataOl(&ol->sendBuff, ol);
			if ((res == SOCKET_ERROR) && (WSAGetLastError() != WSA_IO_PENDING))
			{
				--opCounter;
				if (ol->DecrementRefCount())
				{
					FreeSendBuffer(sendBuff, opType);
					sendOlPoolSingle.destruct(ol);
				}
			}
		}
	}
	else
	{
		WSABufExt sendBuff = CreateSendBuffer(nBytes, (char*)data, opType, compType);
		if (!sendBuff.head)
			return;

		OverlappedSend* ol = sendOlPoolAll.construct<OverlappedSend>(nPcs);
		opCounter += nPcs;

		for (UINT i = 0; i < nPcs; i++)
		{
			Socket& pc = pcs[i];
			bool isCon = pc.IsConnected();
			if (isCon)
			{
				ol[i].InitInstance(opType, sendBuff, ol);
				res = pc.SendDataOl(&ol[i].sendBuff, ol);
			}
			if (!isCon || ((res == SOCKET_ERROR) && (WSAGetLastError() != WSA_IO_PENDING)))
			{
				--opCounter;
				if (ol->DecrementRefCount())
				{
					sendOlPoolAll.dealloc(ol);
					FreeSendBuffer(sendBuff, opType);
				}
			}
		}
	}
}

void TCPServ::SendMsg(Socket exAddr, bool single, short type, short message)
{
	short msg[] = { type, message };

	SendClientData((const char*)msg, MSG_OFFSET, exAddr, single, OpType::sendmsg, NOCOMPRESSION);
}
void TCPServ::SendMsg(Socket* pcs, UINT nPcs, short type, short message)
{
	short msg[] = { type, message };

	SendClientData((const char*)msg, MSG_OFFSET, pcs, nPcs, OpType::sendmsg, NOCOMPRESSION);
}
void TCPServ::SendMsg(std::vector<Socket>& pcs, short type, short message)
{
	SendMsg(pcs.data(), (USHORT)pcs.size(), type, message);
}
void TCPServ::SendMsg(const std::tstring& user, short type, short message)
{
	SendMsg(FindClient(user)->pc, true, type, message);
}

void TCPServ::RecvDataCR(DWORD bytesTrans, ClientDataEx& cd, OverlappedExt* ol)
{
	char* ptr = cd.buff.head;
	while (true)
	{
		BufSize bufSize(*(DWORD64*)ptr);
		const DWORD bytesToRecv = ((bufSize.up.nBytesComp) ? bufSize.up.nBytesComp : bufSize.up.nBytesDecomp);

		//If there is a full data block ready for processing
		if (bytesToRecv >= bytesTrans)
		{
			ptr += sizeof(DWORD64);
			cd.buff.curBytes += min(bytesTrans, bytesToRecv + sizeof(DWORD64));
			//If data was compressed
			if (bufSize.up.nBytesComp)
			{
				const DWORD maxCompSize = cd.serv.MaxCompSize();
				BYTE* dest = (BYTE*)(cd.buff.head + sizeof(DWORD64) + maxCompSize);
				FileMisc::Decompress(dest, maxCompSize, (const BYTE*)ptr, bytesToRecv);	// Decompress data
				(cd.func)(cd.serv, &cd, dest, bufSize.up.nBytesDecomp);
			}
			//If data was not compressed
			else
			{
				(cd.func)(cd.serv, &cd, (const BYTE*)ptr, bufSize.up.nBytesDecomp);
			}
			//If no partial blocks to copy to start of buffer
			if (cd.buff.curBytes == bytesTrans)
			{
				cd.buff.buf = cd.buff.head;
				cd.buff.curBytes = 0;
				break;
			}
		}
		//If the next block of data has not been fully received
		else
		{
			//Concatenate remaining data to buffer
			const DWORD temp = bytesTrans - cd.buff.curBytes;
			if (cd.buff.head != ptr)
				memcpy(cd.buff.head, ptr, temp);
			cd.buff.curBytes = temp;
			cd.buff.buf = cd.buff.head + temp;
			break;
		}
		ptr += bytesToRecv;
	}

	cd.pc.ReadDataOl(&cd.buff, &cd.ol);
}
void TCPServ::AcceptConCR(HostSocket& host, OverlappedExt* ol)
{
	sockaddr *localAddr, *remoteAddr;
	int localLen, remoteLen;

	//Separate addresses and create new socket
	Socket::GetAcceptExAddrs(host.buffer, host.localAddrLen, host.remoteAddrLen, &localAddr, &localLen, &remoteAddr, &remoteLen);
	Socket socket = host.accept;
	socket.SetAddrInfo(remoteAddr, false);

	//Get ready for another call
	host.SetAcceptSocket();

	//Queue another acceptol
	host.listen.AcceptOl(host.accept, host.buffer, host.localAddrLen, host.remoteAddrLen, ol);

	//If server is not full and has passed the connection condition test
	if (!MaxClients() && connectionCondition(*this, socket))
		host.serv.AddClient(socket); //Add client to server list
	else
		socket.Disconnect();
}
void TCPServ::SendDataCR(ClientDataEx& cd, OverlappedSend* ol)
{
	//Decrement refcount
	if (ol->DecrementRefCount())
	{
		//Free buffers
		cd.serv.FreeSendBuffer(ol->sendBuff, ol->opType);
		cd.serv.FreeSendOverlapped(ol);
	}

	if ((--opCounter).GetOpCount() == 0)
		SetEvent(shutdownEv);
}
void TCPServ::CleanupAcceptEx(HostSocket& host)
{
	//Cleanup host and accept sockets
	host.listen.Disconnect();
	host.CloseAcceptSocket();

	if ((--opCounter).GetOpCount() == 0)
		SetEvent(shutdownEv);
}


TCPServ::TCPServ(sfunc func, ConFunc conFunc, DisconFunc disFunc, DWORD nThreads, DWORD nConcThreads, UINT maxDataSize, UINT maxCon, int compression, int compressionCO, float keepAliveInterval, bool noDelay, void* obj)
	:
	ipv4Host(*this),
	ipv6Host(*this),
	clients(nullptr),
	nClients(0),
	iocp(nThreads, nConcThreads, IOCPThread),
	function(func),
	obj(obj),
	conFunc(conFunc),
	connectionCondition(nullptr),
	disFunc(disFunc),
	maxDataSize(maxDataSize),
	maxCompSize(FileMisc::GetCompressedBufferSize(maxDataSize)),
	maxBufferSize(sizeof(DWORD64) + maxDataSize + maxCompSize),
	compression(compression),
	compressionCO(compressionCO),
	maxCon(maxCon),
	keepAliveInterval(keepAliveInterval),
	keepAliveHandler(nullptr),
	clientPool(sizeof(ClientDataEx), maxCon),
	recvBuffPool(maxBufferSize, maxCon),
	sendOlPoolSingle(sizeof(OverlappedSend), maxCon),
	sendOlPoolAll(sizeof(OverlappedSend), 5),
	sendDataPool(maxBufferSize + sizeof(DWORD64), maxCon * 2), //extra DWORD64 incase it sends compressed data, because data written to initial buffer is offset by sizeof(DWORD64)
	sendMsgPool(sizeof(DWORD64) + MSG_OFFSET, maxCon * 2),
	opCounter(),
	shutdownEv(NULL),
	noDelay(noDelay)
{}
TCPServ::TCPServ(TCPServ&& serv)
	:
	ipv4Host(std::move(serv.ipv4Host)),
	ipv6Host(std::move(serv.ipv6Host)),
	clients(serv.clients), 
	nClients(serv.nClients),
	iocp(std::move(serv.iocp)),
	function(serv.function),
	obj(serv.obj),
	conFunc(serv.conFunc),
	connectionCondition(serv.connectionCondition),
	disFunc(serv.disFunc),
	clientSect(serv.clientSect),
	maxDataSize(serv.maxDataSize),
	maxCompSize(serv.maxCompSize),
	maxBufferSize(serv.maxBufferSize),
	compression(serv.compression),
	compressionCO(serv.compressionCO),
	maxCon(serv.maxCon),
	keepAliveHandler(std::move(serv.keepAliveHandler)),
	clientPool(std::move(serv.clientPool)),
	recvBuffPool(std::move(serv.recvBuffPool)),
	sendOlPoolSingle(std::move(serv.sendOlPoolSingle)),
	sendOlPoolAll(std::move(serv.sendOlPoolAll)),
	sendDataPool(std::move(serv.sendDataPool)),
	sendMsgPool(std::move(serv.sendMsgPool)),
	opCounter(serv.opCounter),
	shutdownEv(serv.shutdownEv),
	noDelay(serv.noDelay)
{
	ZeroMemory(&serv, sizeof(TCPServ));
}
TCPServ& TCPServ::operator=(TCPServ&& serv)
{
	if(this != &serv)
	{
		this->~TCPServ();

		ipv4Host = std::move(serv.ipv4Host);
		ipv6Host = std::move(serv.ipv6Host);
		clients = serv.clients;
		nClients = serv.nClients;
		iocp = std::move(serv.iocp);
		function = serv.function;
		obj = serv.obj;
		const_cast<std::remove_const_t<ConFunc>&>(conFunc) = serv.conFunc;
		connectionCondition = serv.connectionCondition;
		const_cast<std::remove_const_t<DisconFunc>&>(disFunc) = serv.disFunc;
		clientSect = serv.clientSect;
		const_cast<UINT&>(maxDataSize) = serv.maxDataSize;
		const_cast<UINT&>(maxCompSize) = serv.maxCompSize;
		const_cast<UINT&>(maxBufferSize) = serv.maxBufferSize;
		const_cast<int&>(compression) = serv.compression;
		const_cast<int&>(compressionCO) = serv.compressionCO;
		const_cast<UINT&>(maxCon) = serv.maxCon;
		keepAliveHandler = std::move(serv.keepAliveHandler);
		clientPool = std::move(serv.clientPool);
		recvBuffPool = std::move(serv.recvBuffPool);
		sendOlPoolSingle = std::move(serv.sendOlPoolSingle);
		sendOlPoolAll = std::move(serv.sendOlPoolAll);
		sendDataPool = std::move(serv.sendDataPool);
		sendMsgPool = std::move(serv.sendMsgPool);
		opCounter = serv.opCounter;
		shutdownEv = serv.shutdownEv;
		noDelay = serv.noDelay;

		ZeroMemory(&serv, sizeof(TCPServ));
	}
	return *this;
}

TCPServ::~TCPServ()
{
	Shutdown();
}

IPv TCPServ::AllowConnections(const LIB_TCHAR* port, ConCondition connectionCondition, IPv ipv)
{
	if(ipv4Host.listen.IsConnected() || ipv6Host.listen.IsConnected())
		return ipvnone;

	if(!clients)
		clients = alloc<ClientDataEx*>(maxCon);

	this->connectionCondition = connectionCondition;

	if (ipv & ipv4)
	{
		if (ipv4Host.listen.Bind(port, false))
		{
			++opCounter; //For AcceptEx
			ipv4Host.SetAcceptSocket();
			ipv4Host.buffer = alloc<char>(ipv4Host.localAddrLen + ipv4Host.remoteAddrLen);
			iocp.LinkHandle((HANDLE)ipv4Host.listen.GetSocket(), &ipv4Host);
			ipv4Host.listen.AcceptOl(ipv4Host.accept, ipv4Host.buffer, ipv4Host.localAddrLen, ipv4Host.remoteAddrLen, &ipv4Host.ol);
		}
		else
		{
			ipv = (IPv)(ipv ^ ipv4);
		}
	}
	if (ipv & ipv6)
	{
		if (ipv6Host.listen.Bind(port, true))
		{
			++opCounter; //For AcceptEx
			ipv6Host.SetAcceptSocket();
			ipv6Host.buffer = alloc<char>(ipv6Host.localAddrLen + ipv6Host.remoteAddrLen); 
			iocp.LinkHandle((HANDLE)ipv6Host.listen.GetSocket(), &ipv6Host);
			ipv6Host.listen.AcceptOl(ipv6Host.accept, ipv6Host.buffer, ipv6Host.localAddrLen, ipv6Host.remoteAddrLen, &ipv6Host.ol);
		}
		else
		{
			ipv = (IPv)(ipv ^ ipv6);
		}
	}
	
	keepAliveHandler = construct<KeepAliveHandler>(this);

	if (!keepAliveHandler)
		return ipvnone;

	keepAliveHandler->SetKeepAliveTimer(keepAliveInterval);

	InitializeCriticalSection(&clientSect);

	shutdownEv = CreateEvent(NULL, TRUE, FALSE, NULL);

	return ipv;
}

void TCPServ::AddClient(Socket pc)
{
	++opCounter; //For recv

	EnterCriticalSection(&clientSect);

	ClientDataEx* cd = clients[nClients] = clientPool.construct<ClientDataEx>(*this, pc, function, nClients);
	nClients += 1;

	LeaveCriticalSection(&clientSect);

	iocp.LinkHandle((HANDLE)pc.GetSocket(), cd);
	pc.SetNoDelay(noDelay);
	pc.ReadDataOl(&cd->buff, &cd->ol);

	RunConFunc(cd);
}
void TCPServ::RemoveClient(ClientDataEx* client, bool unexpected)
{
	RunDisFunc(client, unexpected);

	EnterCriticalSection(&clientSect);

	const UINT index = client->arrayIndex;
	ClientDataEx*& data = clients[index];

	data->pc.Disconnect();
	clientPool.destruct(data);

	if(index != (nClients - 1))
	{
		data = clients[nClients - 1];
		data->arrayIndex = index;
	}

	nClients -= 1;

	LeaveCriticalSection(&clientSect);

	if ((--opCounter).GetOpCount() == 0)
		SetEvent(shutdownEv);

	//if(!user.empty() && !(ipv4Host.listen.IsConnected() || ipv6Host.listen.IsConnected()))//if user wasnt declined authentication, and server was not shut down
	//{
	//	MsgStreamWriter streamWriter(TYPE_CHANGE, MSG_CHANGE_DISCONNECT, StreamWriter::SizeType(user));
	//	streamWriter.Write(user);
	//	SendClientData(streamWriter, streamWriter.GetSize(), Socket(), false);
	//}
}

TCPServ::ClientData* TCPServ::FindClient(const std::tstring& user) const
{
	for (UINT i = 0; i < nClients; i++)
	{
		if (clients[i]->user.compare(user) == 0)
			return clients[i];
	}
	return nullptr;
}

void TCPServ::DisconnectClient(ClientData* client)
{
	ClientDataEx* clint = (ClientDataEx*)client;
	clint->state = ClientDataEx::closing;
	clint->pc.Disconnect();
}

void TCPServ::Shutdown()
{
	if (shutdownEv)
	{
		//Stop sending keepalive packets
		destruct(keepAliveHandler);

		//Stop accepting new connections
		ipv4Host.listen.Disconnect();
		ipv6Host.listen.Disconnect();

		//Cancel all operations
		for (ClientDataEx **ptr = clients, **end = clients + nClients; ptr != end; ptr++)
			(*ptr)->pc.Disconnect();

		//Wait for all operations to cease
		WaitForSingleObject(shutdownEv, INFINITE);

		//Post a close message to all iocp threads
		iocp.Post(0, nullptr);

		//Wait for iocp threads to close, then cleanup iocp
		iocp.WaitAndCleanup();

		//Free up client array
		dealloc(clients);

		DeleteCriticalSection(&clientSect);

		CloseHandle(shutdownEv);
		shutdownEv = NULL;
	}
}

void TCPServ::KeepAlive()
{
	for (ClientDataEx **ptr = clients, **end = clients + nClients; ptr != end; ptr++)
		(*ptr)->pc.SendData(nullptr, 0);
}

ClientAccess TCPServ::GetClients() const
{
	return (ClientData**)clients;
}
UINT TCPServ::ClientCount() const
{
	return nClients;
}
UINT TCPServ::MaxClientCount() const
{
	return maxCon;
}

Socket TCPServ::GetHostIPv4() const
{
	return ipv4Host.listen;
}
Socket TCPServ::GetHostIPv6() const
{
	return ipv6Host.listen;
}

void TCPServ::SetKeepAliveInterval(float interval)
{
	keepAliveInterval = interval;
	keepAliveHandler->SetKeepAliveTimer(keepAliveInterval);
}
float TCPServ::GetKeepAliveInterval() const
{
	return keepAliveInterval;
}

void TCPServ::RunConFunc(ClientData* client)
{
	conFunc(client);
}
void TCPServ::RunDisFunc(ClientData* client, bool unexpected)
{
	disFunc(client, unexpected);
}

void* TCPServ::GetObj() const
{
	return obj;
}

int TCPServ::GetCompression() const
{
	return compression;
}
int TCPServ::GetCompressionCO() const
{
	return compressionCO;
}

bool TCPServ::MaxClients() const
{
	return nClients == maxCon;
}
bool TCPServ::IsConnected() const
{
	return ipv4Host.listen.IsConnected() || ipv6Host.listen.IsConnected();
}
bool TCPServ::NoDelay() const
{
	return noDelay;
}

UINT TCPServ::MaxDataSize() const
{
	return maxDataSize;
}
UINT TCPServ::MaxCompSize() const
{
	return maxCompSize;
}
UINT TCPServ::GetOpCount() const
{
	return opCounter.GetOpCount();
}
UINT TCPServ::MaxBufferSize() const
{
	return sizeof(DWORD64) + maxDataSize + maxCompSize;
}

InterlockedCounter& TCPServ::GetOpCounter()
{
	return opCounter;
}

IOCP& TCPServ::GetIOCP()
{
	return iocp;
}

MemPool<HeapAllocator>& TCPServ::GetRecvBuffPool()
{
	return recvBuffPool;
}
