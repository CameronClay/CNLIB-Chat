#pragma once
#include "CNLIB\TCPServInterface.h"
#include <unordered_map>
#include "ColorDef.h"
#include "WhiteboardClientData.h"
#include "Palette.h"
#include "Timer.h"
#include "Vec2.h"

class Whiteboard
{
public:
	Whiteboard(TCPServInterface &serv, WBParams params, std::tstring creator);
	Whiteboard(Whiteboard &&wb);

	~Whiteboard();

	void StartThread();

	void SendBitmap(const RectU& rect);
	void SendBitmap(const RectU& rect, const Socket& sock, bool single);
	void QueueSendBitmap(const RectU& rect, const Socket& sock, bool beginFrame, bool endFrame);

	void Draw();

	BYTE* GetBitmap();
	CRITICAL_SECTION* GetMapSect();
	std::unordered_map<Socket, WBClientData, Socket::Hash>& GetMap();
	std::vector<Socket>& GetPcs();
	bool IsCreator(const std::tstring& user) const;
	void AddClient(Socket pc);
	void RemoveClient(Socket pc);
	const WBParams& GetParams() const;
	const Palette& GetPalette() const;
	HANDLE GetTimer() const;
	WBClientData& GetClientData(Socket pc);

	class SendThread
	{
	public:
		SendThread(Whiteboard& wb)
			:
			wb(wb),
			rect(),
			sock(),
			beginFrame(false),
			endFrame(false),
			sendThreadEv(CreateEvent(NULL, TRUE, FALSE, NULL)),
			wbThreadEv(CreateEvent(NULL, TRUE, TRUE, NULL)),
			sendThread(NULL),
			sendThreadID(NULL)
		{}
		SendThread(SendThread&& thread, Whiteboard& wb)
			:
			wb(wb),
			rect(thread.rect),
			sock(thread.sock),
			beginFrame(thread.beginFrame),
			endFrame(thread.endFrame),
			sendThreadEv(thread.sendThreadEv),
			wbThreadEv(thread.wbThreadEv),
			sendThread(thread.sendThread),
			sendThreadID(thread.sendThreadID)
		{
			ZeroMemory(&thread, sizeof(SendThread));
		}
		~SendThread()
		{
			Exit();
		}

		void SingleSendThread()
		{
			SetEvent(sendThreadEv);
		}
		void ResetWbEv()
		{
			ResetEvent(wbThreadEv);
		}
		void Start(LPTHREAD_START_ROUTINE entry)
		{
			sendThread = CreateThread(NULL, 0, entry, this, NULL, &sendThreadID);
		}
		void Exit()
		{
			if(sendThreadEv)
			{
				ResetEvent(sendThreadEv);
				CloseHandle(sendThreadEv);
				sendThreadEv = NULL;
			}
			if(wbThreadEv)
			{
				ResetEvent(wbThreadEv);
				CloseHandle(wbThreadEv);
				wbThreadEv = NULL;
			}
			if(sendThread)
			{
				PostThreadMessage(sendThreadID, WM_QUIT, NULL, NULL);
				WaitForSingleObject(sendThread, INFINITE);
				sendThread = NULL;
			}
		}

		void SetRect(const RectU& rectangle)
		{
			rect = rectangle;
		}
		void SetSinglePC(const Socket& socket)
		{
			sock = socket;
		}
		void SetBeginEnd(bool begin, bool end)
		{
			beginFrame = begin;
			endFrame = end;
		}

		const RectU& GetRect() const
		{
			return rect;
		}
		Whiteboard& GetWhiteboard()
		{
			return wb;
		}
		const Socket& GetSocket() const
		{
			return sock;
		}
		HANDLE GetSendThreadEv() const
		{
			return sendThreadEv;
		}
		HANDLE GetWbThreadEv() const
		{
			return wbThreadEv;
		}
		bool GetBegin() const
		{
			return beginFrame;
		}
		bool GetEnd() const
		{
			return endFrame;
		}
	private:
		Whiteboard& wb;
		RectU rect;
		Socket sock;
		bool beginFrame, endFrame;
		HANDLE sendThreadEv, wbThreadEv;

		HANDLE sendThread;
		DWORD sendThreadID;
	};

private:
	void PutPixel(const PointU& point, BYTE clr);
	void PutPixel(int x, int y, BYTE clr);

	void DrawFlatTriangle(float y0, float y1, float m0, float b0, float m1, float b1, BYTE clr);
	void DrawTriangle(Vec2 v0, Vec2 v1, Vec2 v2, BYTE clr);


	void PaintBrush(WBClientData& clientData, bool begin, bool end);
	void DrawLine(const PointU& p1, const PointU& p2, BYTE clr);
	void DrawQuadrilateral(const Vec2* vertices, BYTE clr);

	Vec2 ModifyPoint(const Vec2& vect);

	RectU ResetRect(const Vec2& p0, const Vec2& p1) const;
	void ModifyRect(RectU& rect, const Vec2& p0, const Vec2& p1);

	UINT GetBufferLen(const RectU& rec) const;
	void MakeRectPixels(const RectU& rect, char* ptr);

	size_t FirstClientWD();
	size_t LastClientWD();
private:
	BYTE *pixels;
	WBParams params;
	CRITICAL_SECTION mapSect;
	TCPServInterface &serv;
	Palette palette;
	std::tstring creator;

	const float interval;
	HANDLE timer, thread;
	DWORD threadID;

	SendThread sendThread;

	std::unordered_map<Socket, WBClientData, Socket::Hash> clients;
	std::vector<Socket> sendPcs;
};
