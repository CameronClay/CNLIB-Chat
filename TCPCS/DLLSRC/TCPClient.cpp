#include "StdAfx.h"
#include "TCPClient.h"
#include "CNLIB/HeapAlloc.h"
#include "CNLIB/BufSize.h"
#include "CNLIB/File.h"

TCPClientInterface* CreateClient(cfunc msgHandler, dcfunc disconFunc, DWORD nThreads, DWORD nConcThreads, UINT maxSendOps, UINT maxDataSize, UINT olCount, UINT sendBuffCount, UINT sendMsgBuffCount, UINT maxCon, int compression, int compressionCO, float keepAliveInterval, bool useOwnBuf, bool noDelay, void* obj)
{
	return construct<TCPClient>(msgHandler, disconFunc, nThreads, nConcThreads, maxSendOps, maxDataSize, olCount, sendBuffCount, sendMsgBuffCount, maxCon, compression, compressionCO, keepAliveInterval, useOwnBuf, noDelay, obj);
}

void DestroyClient(TCPClientInterface*& client)
{
	destruct((TCPClient*&)client);
}

static DWORD CALLBACK IOCPThread(LPVOID info)
{
	IOCP& iocp = *(IOCP*)info;
	DWORD bytesTrans = 0;
	OverlappedExt* ol = nullptr;
	TCPClient* key = NULL;

	bool res = false;
	do
	{
		res = GetQueuedCompletionStatus(iocp.GetHandle(), &bytesTrans, (PULONG_PTR)&key, (OVERLAPPED**)&ol, INFINITE);
		if (res)
		{
			if (!ol)
				break;
			switch (ol->opType)
			{
			case OpType::recv:
				{
					if (bytesTrans == 0)
					{
						key->CleanupRecvData();
						continue;
					}
					key->RecvDataCR(bytesTrans, ol);
				}
				break;
			case OpType::sendsingle:
				key->SendDataCR((OverlappedSendSingle*)ol);
				break;
			}
		}
		else
		{
			if (ol && key)
			{
				//temporary fix
				res = true;
				if (ol->opType == OpType::sendsingle)
				{
					key->SendDataCR((OverlappedSendSingle*)ol);
				}
				else if (ol->opType == OpType::recv)
				{
					key->CleanupRecvData();
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

void TCPClient::FreeSendOl(OverlappedSendSingle* ol)
{
	FreeSendBuffer(ol->sendBuff);
	olPool.dealloc(ol);

	if ((--opCounter).GetOpCount() == 0)
		SetEvent(shutdownEv);
}

void TCPClient::RecvDataCR(DWORD bytesTrans, OverlappedExt* ol)
{
	char* ptr = recvBuff.head;

	//Incase there is already a partial block in buffer
	bytesTrans += recvBuff.curBytes;
	recvBuff.curBytes = 0;

	while (true)
	{
		BufSize bufSize(*(DWORD64*)ptr);
		const DWORD bytesToRecv = ((bufSize.up.nBytesComp) ? bufSize.up.nBytesComp : bufSize.up.nBytesDecomp);

		//If there is a full data block ready for processing
		if (bytesTrans - sizeof(DWORD64) >= bytesToRecv)
		{
			const DWORD temp = bytesToRecv + sizeof(DWORD64);
			recvBuff.curBytes += temp;
			bytesTrans -= temp;
			ptr += sizeof(DWORD64);

			//If data was compressed
			if (bufSize.up.nBytesComp)
			{
				BYTE* dest = (BYTE*)(recvBuff.head + sizeof(DWORD64) + maxCompSize);
				if (FileMisc::Decompress(dest, maxCompSize, (const BYTE*)ptr, bytesToRecv) != UINT_MAX)	// Decompress data
					(function)(*this, MsgStreamReader{ (char*)dest, bufSize.up.nBytesDecomp - MSG_OFFSET });
				else
					Shutdown();  //Shutdown client because decompress failing is an unrecoverable error
			}
			//If data was not compressed
			else
			{
				(function)(*this, MsgStreamReader{ (char*)ptr, bufSize.up.nBytesDecomp - MSG_OFFSET });
			}
			//If no partial blocks to copy to start of buffer
			if (!bytesTrans)
			{
				recvBuff.buf = recvBuff.head;
				recvBuff.curBytes = 0;
				break;
			}
		}
		//If the next block of data has not been fully received
		else if (bytesToRecv <= maxDataSize)
		{
			//Concatenate remaining data to buffer
			const DWORD bytesReceived = bytesTrans - recvBuff.curBytes;
			if (recvBuff.head != ptr)
				memcpy(recvBuff.head, ptr, bytesReceived);
			recvBuff.curBytes = bytesReceived;
			recvBuff.buf = recvBuff.head + bytesReceived;
			break;
		}
		else
		{
			assert(false);
			//error has occured
			//To do: have client ask for maximum transfer size
			break;
		}
		ptr += bytesToRecv;
	}

	host.ReadDataOl(&recvBuff, &recvOl);
}
void TCPClient::SendDataCR(OverlappedSendSingle* ol)
{
	FreeSendOl(ol);

	//If there are per client operations pending then attempt to complete them
	if (!opsPending.empty())
		SendServData(opsPending.front(), true);
}

void TCPClient::CleanupRecvData()
{
	if ((--opCounter).GetOpCount() == 0)
		SetEvent(shutdownEv);
}

TCPClient::TCPClient(cfunc func, dcfunc disconFunc, DWORD nThreads, DWORD nConcThreads, UINT maxSendOps, UINT maxDataSize, UINT olCount, UINT sendBuffCount, UINT sendMsgBuffCount, UINT maxCon, int compression, int compressionCO, float keepAliveInterval, bool useOwnBuf, bool noDelay, void* obj)
	:
	function(func),
	disconFunc(disconFunc),
	iocp(nThreads, nConcThreads, IOCPThread),
	maxDataSize(maxDataSize),
	maxCompSize(FileMisc::GetCompressedBufferSize(maxDataSize)),
	maxSendOps(maxSendOps),
	compression(compression),
	compressionCO(compressionCO),
	unexpectedShutdown(true),
	recvBuff(), //only need maxDataSize * 2 because if compressed data size is > than non compressed, it sends noncompressed
	recvOl(OpType::recv),
	keepAliveInterval(keepAliveInterval),
	keepAliveHandler(nullptr),
	sendDataPool(maxDataSize + maxCompSize + sizeof(DWORD64) * 2, sendBuffCount),
	sendMsgPool(sizeof(DWORD64) + MSG_OFFSET, sendMsgBuffCount),
	olPool(sizeof(OverlappedSendSingle), maxSendOps),
	shutdownEv(NULL),
	noDelay(noDelay),
	useOwnBuf(useOwnBuf),
	obj(obj)
{
	recvBuff.Initialize(maxDataSize, alloc<char>(sizeof(DWORD64) + maxDataSize * 2));
}

TCPClient::TCPClient(TCPClient&& client)
	:
	host(client.host),
	function(client.function),
	disconFunc(client.disconFunc),
	iocp(std::move(client.iocp)),
	maxDataSize(client.maxDataSize),
	maxCompSize(client.maxCompSize),
	maxSendOps(client.maxSendOps),
	compression(client.compression),
	compressionCO(client.compressionCO),
	unexpectedShutdown(client.unexpectedShutdown),
	recvBuff(client.recvBuff), //only need maxDataSize * 2 because if compressed data size is > than non compressed, it sends noncompressed
	recvOl(client.recvOl),
	keepAliveInterval(client.keepAliveInterval),
	keepAliveHandler(std::move(client.keepAliveHandler)),
	sendDataPool(std::move(client.sendDataPool)),
	sendMsgPool(std::move(client.sendMsgPool)),
	olPool(std::move(client.olPool)),
	opsPending(std::move(client.opsPending)),
	opCounter(client.opCounter),
	shutdownEv(client.shutdownEv),
	noDelay(client.noDelay),
	useOwnBuf(client.useOwnBuf),
	obj(client.obj)
{
	ZeroMemory(&client, sizeof(TCPClient));
}

TCPClient& TCPClient::operator=(TCPClient&& client)
{
	if(this != &client)
	{
		this->~TCPClient();

		host = client.host;
		function = client.function;
		(void(*)(bool))disconFunc = client.disconFunc;
		iocp = std::move(client.iocp);
		const_cast<UINT&>(maxDataSize) = client.maxDataSize;
		const_cast<UINT&>(maxCompSize) = client.maxCompSize;
		const_cast<UINT&>(maxSendOps) = client.maxSendOps;
		const_cast<int&>(compression) = client.compression;
		const_cast<int&>(compressionCO) = client.compressionCO;
		unexpectedShutdown = client.unexpectedShutdown;
		recvBuff = client.recvBuff;
		recvOl = client.recvOl;
		keepAliveInterval = client.keepAliveInterval;
		keepAliveHandler = std::move(client.keepAliveHandler);
		sendDataPool = std::move(client.sendDataPool);
		sendMsgPool = std::move(client.sendMsgPool);
		olPool = std::move(client.olPool);
		opsPending = std::move(client.opsPending);
		opCounter = client.opCounter;
		shutdownEv = client.shutdownEv;
		noDelay = client.noDelay;
		useOwnBuf = client.useOwnBuf;
		obj = client.obj;

		ZeroMemory(&client, sizeof(TCPClient));
	}
	return *this;
}

TCPClient::~TCPClient()
{
	Shutdown();
}

bool TCPClient::Connect(const LIB_TCHAR* dest, const LIB_TCHAR* port, bool ipv6, float timeOut)
{
	if(host.IsConnected())
		return false;

	//reset so it will send correct message
	SetShutdownReason(true);

	return host.Connect(dest, port, ipv6, timeOut);
}

void TCPClient::Disconnect()
{
	SetShutdownReason(false);
	host.Disconnect(); //causes iocp thread to close
}

void TCPClient::Shutdown()
{
	if (shutdownEv)
	{
		//Stop sending keepalive packets
		destruct(keepAliveHandler);

		//Cancel all outstanding operations
		Disconnect();

		//Wait for all operations to cease
		WaitForSingleObject(shutdownEv, INFINITE);

		//Post a close message to all iocp threads
		iocp.Post(0, nullptr);

		//Wait for iocp threads to close, then cleanup iocp
		iocp.WaitAndCleanup();

		//Free up client resources
		dealloc(recvBuff.head);

		CloseHandle(shutdownEv);
		shutdownEv = NULL;
	}
}

MsgStreamWriter TCPClient::CreateOutStream(short type, short msg)
{
	return{ GetSendBuffer(), maxDataSize, type, msg };
}
char* TCPClient::GetSendBuffer()
{
	return sendDataPool.alloc<char>() + sizeof(DWORD64);
}

WSABufSend TCPClient::CreateSendBuffer(DWORD nBytesDecomp, char* buffer, bool msg, CompressionType compType)
{
	DWORD nBytesComp = 0, nBytesSend = nBytesDecomp + sizeof(DWORD64);
	char* dest;

	if (msg)
	{
		char* temp = buffer;
		dest = buffer = sendMsgPool.alloc<char>();
		*(int*)(buffer + sizeof(DWORD64)) = *(int*)temp;
	}
	else
	{
		dest = buffer -= sizeof(DWORD64);
		if (nBytesDecomp > maxDataSize)
		{
			sendDataPool.destruct(buffer);
			return{};
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
		DWORD temp = FileMisc::Compress((BYTE*)(buffer + maxDataSize + sizeof(DWORD64)), maxCompSize, (const BYTE*)(buffer + sizeof(DWORD64)), nBytesDecomp, compression);
		if (nBytesComp < nBytesDecomp)
		{
			nBytesComp = temp;
			nBytesSend = nBytesComp;
			dest = buffer + maxDataSize;
		}
	}

	*(DWORD64*)(dest) = ((DWORD64)nBytesDecomp) << 32 | nBytesComp;


	WSABufSend buf;
	buf.Initialize(nBytesSend, dest, buffer);
	return buf;
}
void TCPClient::FreeSendBuffer(WSABufSend& buff)
{
	if (sendDataPool.InPool(buff.head))
		sendDataPool.dealloc(buff.head);
	else
		sendMsgPool.dealloc(buff.head);
}

bool TCPClient::SendServData(const char* data, DWORD nBytes, CompressionType compType)
{
	return SendServData(data, nBytes, false, compType);
}
bool TCPClient::SendServData(MsgStreamWriter streamWriter, CompressionType compType)
{
	return SendServData(streamWriter, streamWriter.GetSize(), false, compType);
}

bool TCPClient::SendServData(const char* data, DWORD nBytes, bool msg, CompressionType compType)
{
	return SendServData(olPool.construct<OverlappedSendSingle>(CreateSendBuffer(nBytes, (char*)data, msg, compType)), false);
}
bool TCPClient::SendServData(OverlappedSendSingle* ol, bool popQueue)
{
	if (host.IsConnected())
	{
		if (opCounter.GetOpCount() - 1 < maxSendOps)
		{
			++opCounter;

			long res = host.SendDataOl(&ol->sendBuff, ol);
			long err = WSAGetLastError();
			if ((res == SOCKET_ERROR) && (err != WSA_IO_PENDING))
			{
				FreeSendOl(ol);
			}
			else if (popQueue)
			{
				opsPending.pop();
			}
		}
		else
		{
			//queue the send for later
			opsPending.emplace(ol);
		}
	}
	else
	{
		FreeSendOl(ol);

		return false;
	}

	return true;
}

void TCPClient::SendMsg(short type, short message)
{
	short msg[] = { type, message };

	SendServData((char*)msg, MSG_OFFSET, true, NOCOMPRESSION);
}
void TCPClient::SendMsg(const std::tstring& name, short type, short message)
{
	MsgStreamWriter streamWriter = CreateOutStream(type, message);
	streamWriter.Write(name);
	SendServData(streamWriter, NOCOMPRESSION);
}

bool TCPClient::RecvServData()
{
	if (!host.IsConnected())
		return false;

	if (noDelay)
		host.SetNoDelay(noDelay);
	if (useOwnBuf)
		host.SetTCPSendStack();

	if (!iocp.LinkHandle((HANDLE)host.GetSocket(), this))
		return false;

	++opCounter; //For recv

	host.ReadDataOl(&recvBuff, &recvOl);

	shutdownEv = CreateEvent(NULL, TRUE, FALSE, NULL);

	keepAliveHandler = construct<KeepAliveHandler>(this);

	if (!keepAliveHandler)
		return false;

	keepAliveHandler->SetKeepAliveTimer(keepAliveInterval);

	return true;
}

void TCPClient::KeepAlive()
{
	host.SendData(nullptr, 0);
}

void TCPClient::SetFunction(cfunc function)
{
	this->function = function;
}

void TCPClient::RunDisconFunc()
{
	disconFunc(unexpectedShutdown);
}

void TCPClient::SetShutdownReason(bool unexpected)
{
	unexpectedShutdown = unexpected;
}

cfuncP TCPClient::GetFunction()
{
	return &function;
}

Socket TCPClient::GetHost() const
{
	return host;
}

void* TCPClient::GetObj() const
{
	return obj;
}

int TCPClient::GetCompression() const
{
	return compression;
}
int TCPClient::GetCompressionCO() const
{
	return compressionCO;
}

dcfunc TCPClient::GetDisfunc() const
{
	return disconFunc;
}

bool TCPClient::IsConnected() const
{
	return host.IsConnected();
}

void TCPClient::SetKeepAliveInterval(float interval)
{
	keepAliveInterval = interval;
	keepAliveHandler->SetKeepAliveTimer(keepAliveInterval);
}
float TCPClient::GetKeepAliveInterval() const
{
	return keepAliveInterval;
}

UINT TCPClient::MaxDataSize() const
{
	return maxDataSize;
}
UINT TCPClient::MaxCompSize() const
{
	return maxCompSize;
}
UINT TCPClient::GetOpCount() const
{
	return opCounter.GetOpCount();
}

UINT TCPClient::GetMaxSendOps() const
{
	return maxSendOps;
}

bool TCPClient::NoDelay() const
{
	return noDelay;
}
bool TCPClient::UseOwnBuf() const
{
	return useOwnBuf;
}
