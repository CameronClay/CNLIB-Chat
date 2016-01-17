#include "TCPServ.h"
#include "HeapAlloc.h"
#include "File.h"
#include "Messages.h"
#include "MsgStream.h"

TCPServInterface* CreateServer(sfunc msgHandler, ConFunc conFunc, DisconFunc disFunc, DWORD nThreads, DWORD nConcThreads, UINT maxDataSize, USHORT maxCon, int compression, int compressionCO, float keepAliveInterval, void* obj)
{
	return construct<TCPServ>(msgHandler, conFunc, disFunc, nThreads, nConcThreads, maxDataSize, maxCon, compression, compressionCO, keepAliveInterval, obj);
}
void DestroyServer(TCPServInterface*& server)
{
	destruct((TCPServ*&)server);
}


ClientAccess::ClientAccess(ClientData** clients)
	:
	clients((const ClientData**)clients)
{}
ClientData* ClientAccess::operator+(USHORT amount)
{
	return *(ClientData**)((ClientDataEx**)clients + amount);
}
ClientData* ClientAccess::operator-(USHORT amount)
{
	return *(ClientData**)((ClientDataEx**)clients - amount);
}
ClientData* ClientAccess::operator[](USHORT index)
{
	return *(ClientData**)((ClientDataEx**)clients + index);
}


struct Data
{
	Data(TCPServ& serv, USHORT pos)
		:
		serv(serv),
		pos(pos)
	{}

	TCPServ& serv;
	USHORT pos;
};

struct SAData
{
	SAData(TCPServ& serv, const char* data, DWORD nBytesDecomp, Socket exAddr, const bool single, CompressionType compType)
		:
		serv(serv),
		data(data),
		nBytesDecomp(nBytesDecomp),
		exAddr(exAddr),
		single(single),
		compType(compType)
	{}

	TCPServ& serv;
	DWORD nBytesDecomp;
	const char* data;
	Socket exAddr;
	bool single;
	CompressionType compType;
};

struct SADataEx
{
	SADataEx(TCPServ& serv, const char* data, DWORD nBytesDecomp, Socket* pcs, USHORT nPcs, CompressionType compType)
		:
		serv(serv),
		data(data),
		nBytesDecomp(nBytesDecomp),
		pcs(pcs),
		nPcs(nPcs),
		compType(compType)
	{}

	TCPServ& serv;
	DWORD nBytesDecomp;
	const char* data;
	Socket* pcs;
	USHORT nPcs;
	CompressionType compType;
};

ClientData::ClientData(TCPServInterface& serv, Socket pc, sfunc func)
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


ClientDataEx::ClientDataEx(TCPServ& serv, Socket pc, sfunc func, USHORT arrayIndex)
	:
	ClientData(serv, pc, func),
	serv((TCPServ&)serv),
	ol(OpType::recv),
	arrayIndex(arrayIndex)
{
	MemPool& pool = serv.GetRecvBuffPool();
	const UINT maxDataSize = serv.MaxDataSize();
	char* temp = pool.alloc<char>(false);
	buff.Initalize(maxDataSize, temp, temp);
}
ClientDataEx::ClientDataEx(ClientDataEx&& clint)
	:
	ClientData(std::forward<ClientData>(*this)),
	serv(clint.serv),
	buff(clint.buff),
	ol(clint.ol),
	arrayIndex(clint.arrayIndex)
{
	ZeroMemory(&clint, sizeof(ClientDataEx));
}
ClientDataEx& TCPServ::ClientDataEx::operator=(ClientDataEx&& data)
{
	if (this != &data)
	{
		this->__super::operator=(std::forward<ClientData>(data));
		buff = data.buff;
		arrayIndex = data.arrayIndex;
		ol = data.ol;

		memset(&data, 0, sizeof(ClientDataEx));
	}

	return *this;
}
ClientDataEx::~ClientDataEx()
{
	serv.GetRecvBuffPool().dealloc(buff.head, false);
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
	DWORD bytesTrans;
	OverlappedExt* ol;
	void* key;
	bool res = GetQueuedCompletionStatus(iocp.GetHandle(), &bytesTrans, (ULONG*)key, (OVERLAPPED**)&ol, INFINITE);
	if (!res && ol)
	{
		if (ol->opType == OpType::recv || ol->opType == OpType::send || ol->opType == OpType::sendmsg)
		{
			OverlappedSend* ols = (OverlappedSend*)ol;
			ClientDataEx* cd = (ClientDataEx*)key;
			//Decrement refCount
			if (ols->DecrementRefCount())
			{
				//Free buffers
				cd->serv.FreeSendBuffer(ols->sendBuff, ols->opType);
				cd->serv.FreeSendOverlapped(ols);
			}
			cd->serv.RemoveClient(cd);
		}
		else
		{
			//Cleanup host and accept sockets
			HostSocket& hostSocket = *(HostSocket*)key;
			hostSocket.listen.Disconnect();
			hostSocket.CloseAcceptSocket();
		}
	}
	else
	{
		//error occured
	}

	switch (ol->opType)
	{
		case OpType::recv:
		{
			ClientDataEx& cd = *(ClientDataEx*)key;
			char* ptr = cd.buff.head;
			while (true)
			{
				BufSize bufSize(*(DWORD64*)cd.buff.head);
				const DWORD bytesToRecv = (bufSize.up.nBytesComp) ? bufSize.up.nBytesComp : bufSize.up.nBytesDecomp;

				//If there is a full data block ready for proccessing
				if (bytesToRecv >= bytesTrans)
				{
					cd.buff.curBytes += min(bytesTrans, bytesToRecv);
					//If data was comprsesed
					if (bufSize.up.nBytesComp)
					{
						const DWORD maxCompSize = cd.serv.MaxCompSize();
						BYTE* dest = (BYTE*)(cd.buff.buf + sizeof(DWORD64) + maxCompSize);
						FileMisc::Decompress(dest, maxCompSize, (const BYTE*)(cd.buff.buf), bytesToRecv);	// Decompress data
						(cd.func)(cd.serv, &cd, dest, bufSize.up.nBytesDecomp);
					}
					//If data was not compressed
					else
					{
						(cd.func)(cd.serv, &cd, (const BYTE*)cd.buff.buf, bufSize.up.nBytesDecomp);
					}
					//If no partial blocks to copy to start of buffer
					if (cd.buff.curBytes == bytesTrans)
					{
						cd.buff.buf = cd.buff.head;
						cd.buff.curBytes = 0;
						break;
					}
				}
				else
				{
					//Concatenate remaining data to buffer
					DWORD temp = cd.buff.curBytes - bytesTrans;
					if (cd.buff.buf != ptr)
						memcpy(cd.buff.buf, ptr, temp);
					cd.buff.curBytes = temp;
					cd.buff.buf = cd.buff.head + temp;
					break;
				}
				ptr += bytesToRecv;
			}

			cd.ol.Reset();
			cd.pc.ReadDataOl(&cd.buff, &cd.ol);

			//if (cd.finished)//If ready to decompress/call msgHandler
			//{
			//	DWORD byComp = cd.bufSize.up.nBytesDecomp, 
			//		byDecomp = cd.bufSize.up.nBytesDecomp;		// Create temporary size vars because buffer cant be accessed while reading
			//	cd.bufSize.size = 0;							// Zero out size of buffer so it doesnt trigger uncompression when it shouldnt
			//	cd.finished = false;							// Set finished state to false
			//	ol->Reset();									// Reset overlapped structure
			//	cd.pc.ReadDataOl(&cd.sizeBuff, &cd.ol); 		// Queue another read

			//	if (byComp)
			//	{
			//		FileMisc::Decompress((BYTE*)cd.decompBuff, byDecomp, (const BYTE*)cd.recvBuff.buf, byComp);	// Decompress data
			//		(cd.func)(cd.serv, &cd, (const BYTE*)cd.decompBuff, byDecomp);								// Call msgHandler
			//	}
			//	else
			//	{
			//		(cd.func)(cd.serv, &cd, (const BYTE*)cd.recvBuff.buf, byDecomp);							// Call msgHandler
			//	}
			//}
			//else //If received number of bytes to receive
			//{
			//	//Setup buffer for next read
			//	if (cd.bufSize.up.nBytesComp)
			//		cd.recvBuff.len = cd.bufSize.up.nBytesComp;
			//	else
			//		cd.recvBuff.len = cd.bufSize.up.nBytesDecomp;

			//	//No need to reset overlapped because it is reset on construction
			//	//Read in nBytes data
			//	cd.pc.ReadDataOl(&cd.recvBuff, &cd.ol);
			//}
		}
		break;
		case OpType::send:
		case OpType::sendmsg:
		{
			OverlappedSend* ols = (OverlappedSend*)ol;
			ClientDataEx& cd = *(ClientDataEx*)key;
			//Decrement refcount
			if (ols->DecrementRefCount())
			{
				//Free buffers
				cd.serv.FreeSendBuffer(ols->sendBuff, ols->opType);
				cd.serv.FreeSendOverlapped(ols);			
			}
		}
		break;
		case OpType::accept:
		{
			HostSocket& hostSocket = *(HostSocket*)key;
			sockaddr *localAddr, *remoteAddr;
			int localLen, remoteLen;

			//Separate addresses and create new socket
			Socket::GetAcceptExAddrs(hostSocket.buffer, hostSocket.localAddrLen, hostSocket.remoteAddrLen, &localAddr, &localLen, &remoteAddr, &remoteLen);
			Socket socket = hostSocket.accept;
			socket.SetAddrInfo(remoteAddr, false);

			//Get ready for another call
			hostSocket.SetAcceptSocket();
			ol->Reset();

			//Queue another acceptol
			hostSocket.listen.AcceptOl(hostSocket.accept, hostSocket.buffer, hostSocket.localAddrLen, hostSocket.remoteAddrLen, ol);

			//Add client to server list
			hostSocket.serv.AddClient(socket);
		}
		break;
	}

	return 0;
}


//static void SendDataComp(Socket pc, const char* data, DWORD nBytesDecomp, DWORD nBytesComp)
//{
//	const DWORD64 nBytes = ((DWORD64)nBytesDecomp) << 32 | nBytesComp;
//
//	//No need to remove client, that is handled in recv thread
//	if (pc.SendData(&nBytes, sizeof(DWORD64)) > 0)
//		pc.SendData(data, (nBytesComp != 0) ? nBytesComp : nBytesDecomp);
//}
//
//static DWORD CALLBACK SendAllData(LPVOID info)
//{
//	SAData* data = (SAData*)info;
//	TCPServ& serv = data->serv;
//	auto clients = serv.GetClients();
//	//CRITICAL_SECTION* sect = serv.GetSendSect();
//	Socket exAddr = data->exAddr;
//	const BYTE* dataDecomp = (const BYTE*)data->data;
//	BYTE* dataComp = nullptr;
//	const DWORD nBytesDecomp = data->nBytesDecomp;
//	DWORD nBytesComp = 0;
//
//	if (data->compType == SETCOMPRESSION)
//	{
//		nBytesComp = FileMisc::GetCompressedBufferSize(nBytesDecomp);
//		dataComp = alloc<BYTE>(nBytesComp);
//		nBytesComp = FileMisc::Compress(dataComp, nBytesComp, dataDecomp, nBytesDecomp, serv.GetCompression());
//	}
//
//	const char* dat = (const char*)(dataComp ? dataComp : dataDecomp);
//
//	//EnterCriticalSection(sect);
//
//	if (data->single)
//	{
//		if (exAddr.IsConnected())
//			SendDataComp(exAddr, dat, nBytesDecomp, nBytesComp);
//	}
//
//	else
//	{
//		const USHORT nClients = serv.ClientCount();
//		if (exAddr.IsConnected())
//		{
//			for (USHORT i = 0; i < nClients; i++)
//				if (clients[i]->pc != exAddr)
//					SendDataComp(clients[i]->pc, dat, nBytesDecomp, nBytesComp);
//		}
//		else
//		{
//			for (USHORT i = 0; i < nClients; i++)
//				SendDataComp(clients[i]->pc, dat, nBytesDecomp, nBytesComp);
//		}
//	}
//
//	//LeaveCriticalSection(sect);
//
//	dealloc(dataComp);
//	destruct(data);
//
//	return 0;
//}
//
//static DWORD CALLBACK SendAllDataEx(LPVOID info)
//{
//	SADataEx* data = (SADataEx*)info;
//	TCPServ& serv = data->serv;
//	Socket* pcs = data->pcs;
//	const USHORT pcCount = data->nPcs;
//	//CRITICAL_SECTION* sect = serv.GetSendSect();
//	const BYTE* dataDecomp = (const BYTE*)data->data;
//	BYTE* dataComp = nullptr;
//	const DWORD nBytesDecomp = data->nBytesDecomp;
//	DWORD nBytesComp = 0;
//
//	if (data->compType == SETCOMPRESSION)
//	{
//		nBytesComp = FileMisc::GetCompressedBufferSize(nBytesDecomp);
//		dataComp = alloc<BYTE>(nBytesComp);
//		nBytesComp = FileMisc::Compress(dataComp, nBytesComp, dataDecomp, nBytesDecomp, serv.GetCompression());
//	}
//
//	const char* dat = (const char*)(dataComp ? dataComp : dataDecomp);
//
//	//EnterCriticalSection(sect);
//
//	for (USHORT i = 0; i < pcCount; i++)
//	{
//		if (pcs[i].IsConnected())
//			SendDataComp(pcs[i], dat, nBytesDecomp, nBytesComp);
//	}
//
//	//LeaveCriticalSection(sect);
//
//	dealloc(dataComp);
//	destruct(data);
//
//	return 0;
//}
//
//static DWORD CALLBACK RecvData(LPVOID info)
//{
//	Data* data = (Data*)info;
//	TCPServ& serv = data->serv;
//	TCPServ::ClientData** clients = serv.GetClients();
//	USHORT& pos = clients[data->pos]->recvIndex;
//	TCPServ::ClientData* clint = clients[pos];
//	Socket& pc = clint->pc;
//	void* obj = serv.GetObj();
//	DWORD64 nBytes = 0;
//
//	while(pc.IsConnected())
//	{
//		if(pc.ReadData(&nBytes, sizeof(DWORD64)) > 0)
//		{
//			const DWORD nBytesDecomp = nBytes >> 32;
//			const DWORD nBytesComp = nBytes & 0xffffffff;
//			BYTE* buffer = alloc<BYTE>(nBytesDecomp + nBytesComp);
//
//			if (pc.ReadData(buffer, (nBytesComp != 0) ? nBytesComp : nBytesDecomp) > 0)
//			{
//				BYTE* dest = buffer + nBytesComp;
//				if (nBytesComp != 0)
//					FileMisc::Decompress(dest, nBytesDecomp, buffer, nBytesComp);
//
//				(clint->func)(serv, clint, dest, nBytesDecomp, obj);
//				dealloc(buffer);
//			}
//			else
//			{
//				dealloc(buffer);
//				break;
//			}
//		}
//		else break;
//	}
//
//	serv.RemoveClient(pos);
//	destruct(data);
//	return 0;
//}

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
		buffer = sendMsgPool.alloc<char>();
		*(short*)buffer = *(short*)temp;
	}
	else
	{
		buffer -= sizeof(DWORD64);
		if (nBytesDecomp > maxDataSize)
		{
			sendDataPool.dealloc(buffer);
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
	buf.Initalize(nBytesSend, dest, buffer);
	return buf;
}
void TCPServ::FreeSendBuffer(WSABufExt buff, OpType opType)
{
	if (opType == OpType::send)
		sendDataPool.dealloc(buff.head, true);
	else
		sendMsgPool.dealloc(buff.head, true);
}
void TCPServ::FreeSendOverlapped(OverlappedSend* ol)
{
	if (sendOlPoolAll.InPool(ol->head))
		sendOlPoolAll.dealloc(ol->head, true);
	else
		sendOlPoolSingle.dealloc(ol->head, true);
}

void TCPServ::SendClientData(const char* data, DWORD nBytes, Socket exAddr, bool single, CompressionType compType)
{
	SendClientData(data, nBytes, exAddr, single, OpType::send, compType);
}
void TCPServ::SendClientData(const char* data, DWORD nBytes, Socket* pcs, USHORT nPcs, CompressionType compType)
{
	SendClientData(data, nBytes, pcs, nPcs, OpType::send, compType);
}
void TCPServ::SendClientData(const char* data, DWORD nBytes, std::vector<Socket>& pcs, CompressionType compType)
{
	SendClientData(data, nBytes, pcs.data(), pcs.size(), compType);
}

void TCPServ::SendClientData(const char* data, DWORD nBytes, Socket exAddr, bool single, OpType opType, CompressionType compType)
{
	WSABufExt sendBuff = CreateSendBuffer(nBytes, (char*)data, opType, compType);
	if (!sendBuff.head)
		return;

	if (single)
	{
		if (exAddr.IsConnected())
		{
			OverlappedSend* ol = sendOlPoolSingle.construct<OverlappedSend>(true);
			ol->InitInstance(opType, sendBuff, ol);
			exAddr.SendDataOl(&ol->sendBuff, ol);
		}
		else
		{
			FreeSendBuffer(sendBuff, opType);
		}
	}
	else
	{
		if (exAddr.IsConnected())
		{
			OverlappedSend* ol = sendOlPoolAll.construct<OverlappedSend>(true, nClients - 1);
			for (USHORT i = 0; i < nClients; i++)
			{
				ClientDataEx& cd = *clients[i];
				if (cd.pc != exAddr)
				{
					if (cd.pc.IsConnected())
					{
						ol[i].InitInstance(opType, sendBuff, ol);
						cd.pc.SendDataOl(&ol[i].sendBuff, ol);
					}
					else if (ol->DecrementRefCount())
					{
						sendOlPoolAll.dealloc(ol, true);
						FreeSendBuffer(sendBuff, opType);
					}
				}
			}
		}
		else
		{
			OverlappedSend* ol = sendOlPoolAll.construct<OverlappedSend>(true, nClients);
			for (USHORT i = 0; i < nClients; i++)
			{
				ClientDataEx& cd = *clients[i];
				if (cd.pc.IsConnected())
				{
					ol[i].InitInstance(opType, sendBuff, ol);
					cd.pc.SendDataOl(&ol[i].sendBuff, ol);
				}
				else if (ol->DecrementRefCount())
				{
					sendOlPoolAll.dealloc(ol, true);
					FreeSendBuffer(sendBuff, opType);
				}
			}
		}
	}
}
void TCPServ::SendClientData(const char* data, DWORD nBytes, Socket* pcs, USHORT nPcs, OpType opType, CompressionType compType)
{
	WSABufExt sendBuff = CreateSendBuffer(nBytes, (char*)data, opType, compType);
	if (!sendBuff.head)
		return;

	if (nPcs == 1)
	{
		if (pcs->IsConnected())
		{
			OverlappedSend* ol = sendOlPoolSingle.construct<OverlappedSend>(true, 1);
			ol->InitInstance(opType, sendBuff, ol);
			pcs->SendDataOl(&ol->sendBuff, ol);
		}
		else
		{
			FreeSendBuffer(sendBuff, opType);
		}
	}
	else
	{
		OverlappedSend* ol = sendOlPoolAll.construct<OverlappedSend>(true, nPcs);
		for (USHORT i = 0; i < nPcs; i++)
		{
			if (pcs[i].IsConnected())
			{
				ol[i].InitInstance(opType, sendBuff, ol);
				pcs[i].SendDataOl(&ol[i].sendBuff, ol);
			}
			else if (ol->DecrementRefCount())
			{
				sendOlPoolAll.dealloc(ol, true);
				FreeSendBuffer(sendBuff, opType);
			}
		}
	}
}

void TCPServ::SendMsg(Socket pc, bool single, short type, short message)
{
	char msg[] = { type, message };

	SendClientData(msg, MSG_OFFSET, pc, single, OpType::sendmsg, NOCOMPRESSION);
}
void TCPServ::SendMsg(Socket* pcs, USHORT nPcs, short type, short message)
{
	char msg[] = { type, message };

	SendClientData(msg, MSG_OFFSET, pcs, nPcs, OpType::sendmsg, NOCOMPRESSION);
}
void TCPServ::SendMsg(std::vector<Socket>& pcs, short type, short message)
{
	SendMsg(pcs.data(), pcs.size(), type, message);
}
void TCPServ::SendMsg(const std::tstring& user, short type, short message)
{
	SendMsg(FindClient(user)->pc, true, type, message);
}


TCPServ::TCPServ(sfunc func, ConFunc conFunc, DisconFunc disFunc, DWORD nThreads, DWORD nConcThreads, UINT maxDataSize, USHORT maxCon, int compression, int compressionCO, float keepAliveInterval, void* obj)
	:
	ipv4Host(*this),
	ipv6Host(*this),
	clients(nullptr),
	nClients(0),
	iocp(nThreads, nConcThreads, IOCPThread),
	function(func),
	obj(obj),
	conFunc(conFunc),
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
	sendOlPoolSingle(sizeof(OverlappedSend), nClients),
	sendOlPoolAll(sizeof(OverlappedSend), 5),
	sendDataPool(maxBufferSize, nClients * 2),
	sendMsgPool(sizeof(DWORD64), nClients * 2)
{
	
}
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
	sendMsgPool(std::move(serv.sendMsgPool))
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
		const_cast<std::remove_const_t<DisconFunc>&>(disFunc) = serv.disFunc;
		clientSect = serv.clientSect;
		const_cast<UINT&>(maxDataSize) = serv.maxDataSize;
		const_cast<UINT&>(maxCompSize) = serv.maxCompSize;
		const_cast<UINT&>(maxBufferSize) = serv.maxBufferSize;
		const_cast<int&>(compression) = serv.compression;
		const_cast<int&>(compressionCO) = serv.compressionCO;
		const_cast<USHORT&>(maxCon) = serv.maxCon;
		keepAliveHandler = std::move(serv.keepAliveHandler);
		clientPool = std::move(serv.clientPool);
		recvBuffPool = std::move(serv.recvBuffPool);
		sendOlPoolSingle = std::move(serv.sendOlPoolSingle);
		sendOlPoolAll = std::move(serv.sendOlPoolAll);
		sendDataPool = std::move(serv.sendDataPool);
		sendMsgPool = std::move(serv.sendMsgPool);

		ZeroMemory(&serv, sizeof(TCPServ));
	}
	return *this;
}

TCPServ::~TCPServ()
{
	Shutdown();
}

IPv TCPServ::AllowConnections(const LIB_TCHAR* port, IPv ipv)
{
	if(ipv4Host.listen.IsConnected() || ipv6Host.listen.IsConnected())
		return ipvnone;

	if(!clients)
		clients = alloc<ClientDataEx*>(maxCon);

	if (ipv & ipv4)
	{
		if (ipv4Host.listen.Bind(port, false))
		{
			ipv4Host.SetAcceptSocket();
			ipv4Host.buffer = alloc<char>(ipv4Host.localAddrLen + ipv4Host.remoteAddrLen);
			iocp.LinkHandle((HANDLE)ipv4Host.listen.GetSocket(), &ipv4Host);
		}
		else
		{
			ipv = (IPv)(ipv ^ ipv4);
		}
	}
	if (ipv & ipv6)
	{
		if (ipv6Host.listen.Bind(port, false))
		{
			ipv6Host.SetAcceptSocket();
			ipv6Host.buffer = alloc<char>(ipv4Host.localAddrLen + ipv4Host.remoteAddrLen); 
			iocp.LinkHandle((HANDLE)ipv6Host.listen.GetSocket(), &ipv4Host);
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

	return ipv;
}

void TCPServ::AddClient(Socket pc)
{
	EnterCriticalSection(&clientSect);

	ClientDataEx* cd = clients[nClients] = clientPool.construct<ClientDataEx>(false, *this, pc, function, nClients);
	++nClients;

	LeaveCriticalSection(&clientSect);

	iocp.LinkHandle((HANDLE)pc.GetSocket(), cd);
	pc.ReadDataOl(&cd->buff, &cd->ol);

	RunConFunc(cd);
}
void TCPServ::RemoveClient(ClientDataEx* client, bool unexpected)
{
	//If client wasn't removed intentionaly
	if (client->pc.IsConnected())
		RunDisFunc(client, unexpected);

	EnterCriticalSection(&clientSect);

	const USHORT index = client->arrayIndex;
	ClientDataEx*& data = clients[index];

	data->pc.Disconnect();
	clientPool.destruct(data);

	if(index != (nClients - 1))
	{
		data = clients[nClients - 1];
		data->arrayIndex = index;
	}

	--nClients;

	LeaveCriticalSection(&clientSect);

	//if(!user.empty() && !(ipv4Host.listen.IsConnected() || ipv6Host.listen.IsConnected()))//if user wasnt declined authentication, and server was not shut down
	//{
	//	MsgStreamWriter streamWriter(TYPE_CHANGE, MSG_CHANGE_DISCONNECT, StreamWriter::SizeType(user));
	//	streamWriter.Write(user);
	//	SendClientData(streamWriter, streamWriter.GetSize(), Socket(), false);
	//}
}

TCPServ::ClientData* TCPServ::FindClient(const std::tstring& user) const
{
	for (USHORT i = 0; i < nClients; i++)
	{
		if (clients[i]->user.compare(user) == 0)
			return clients[i];
	}
	return nullptr;
}

void TCPServ::DisconnectClient(ClientData* client, bool unexpected)
{
	RunDisFunc(client, unexpected);
	client->pc.Disconnect();
}

void TCPServ::Shutdown()
{
	if (keepAliveHandler)
	{
		destruct(keepAliveHandler);

		ipv4Host.listen.Disconnect();
		ipv6Host.listen.Disconnect();

		//nClients != 0 because nClients changes after thread ends
		//close recv threads and free memory
		while(nClients != 0)
		{
			ClientDataEx* data = clients[nClients - 1];
			RemoveClient(data, false);
		}

		dealloc(clients);

		DeleteCriticalSection(&clientSect);
	}
}

void TCPServ::KeepAlive(Socket client)
{
	client.SendData(nullptr, 0);
}
void TCPServ::KeepAlive()
{
	for (ClientDataEx **ptr = clients, **end = clients + nClients; ptr != end; ptr++)
		(*ptr)->pc.SendData(nullptr, 0);
}

bool TCPServ::MaxClients() const
{
	return nClients == maxCon;
}
ClientAccess TCPServ::GetClients() const
{
	return (ClientData**)clients;
}
USHORT TCPServ::ClientCount() const
{
	return nClients;
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

bool TCPServ::IsConnected() const
{
	return ipv4Host.listen.IsConnected() || ipv6Host.listen.IsConnected();
}

UINT TCPServ::MaxDataSize() const
{
	return maxDataSize;
}
UINT TCPServ::MaxCompSize() const
{
	return maxCompSize;
}
UINT TCPServ::MaxBufferSize() const
{
	return sizeof(DWORD64) + maxDataSize + maxCompSize;
}

IOCP& TCPServ::GetIOCP()
{
	return iocp;
}

MemPool& TCPServ::GetRecvBuffPool()
{
	return recvBuffPool;
}
