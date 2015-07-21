#include "Whiteboard.h"
#include "CNLIB\HeapAlloc.h"
#include "CNLIB\Messages.h"
#include <stdlib.h>
#include <time.h>
#include <assert.h>

DWORD CALLBACK WBThread(LPVOID param)
{
	Whiteboard& wb = *(Whiteboard*)param;
	HANDLE timer = wb.GetTimer();

	while(true)
	{
		const DWORD ret = MsgWaitForMultipleObjects(1, &timer, FALSE, INFINITE, QS_ALLINPUT);
		if(ret == WAIT_OBJECT_0)
		{
			wb.Draw();
		}

		else if(ret == WAIT_OBJECT_0 + 1)
		{
			MSG msg;
			while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) != 0)
			{
				if(msg.message == WM_QUIT)
				{
					CancelWaitableTimer(timer);
					CloseHandle(timer);

					return 0;
				}
			}
		}

		else
			return 0;
	}
}

DWORD CALLBACK SendBitmapThread(LPVOID param)
{
	Whiteboard::SendThread& st = *(Whiteboard::SendThread*)param;
	Whiteboard& wb = st.GetWhiteboard();
	HANDLE ev = st.GetSendThreadEv();
	HANDLE wbEv = st.GetWbThreadEv();

	while(true)
	{
		const DWORD ret = MsgWaitForMultipleObjects(1, &ev, FALSE, INFINITE, QS_ALLINPUT);
		if(ret == WAIT_OBJECT_0)
		{
			if(st.GetSocket().IsConnected())
				wb.SendBitmap(st.GetRect(), st.GetSocket(), true);
			else
				wb.SendBitmap(st.GetRect());


			ResetEvent(ev);
			SetEvent(wbEv);
		}

		else if(ret == WAIT_OBJECT_0 + 1)
		{
			MSG msg;
			while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) != 0)
			{
				if(msg.message == WM_QUIT)
					return 0;
			}
		}

		else
			return 0;
	}
}

Whiteboard::Whiteboard(TCPServInterface &serv, WBParams params, std::tstring creator)
	:
pixels(alloc<BYTE>(params.width * params.height)),
params(std::move(params)),
serv(serv),
creator(creator),
interval(1.0f / (float)params.fps),
timer(CreateWaitableTimer(NULL, FALSE, NULL)),
thread(NULL),
threadID(NULL),
sendThread(*this)
{
	FillMemory(pixels, params.width * params.height, params.clrIndex);

	InitializeCriticalSection(&mapSect);
	srand(time(NULL));
}

Whiteboard::Whiteboard(Whiteboard &&wb)
	:
pixels(wb.pixels),
params(std::move(wb.params)),
mapSect(wb.mapSect),
serv(std::move(wb.serv)),
creator(std::move(wb.creator)),
interval(wb.interval),
timer(wb.timer),
thread(wb.thread),
threadID(wb.threadID),
sendThread(std::move(wb.sendThread), *this),
clients(std::move(wb.clients)),
sendPcs(std::move(wb.sendPcs))
{
	wb.pixels = nullptr;
}

BYTE* Whiteboard::GetBitmap()
{
	return pixels;
}

CRITICAL_SECTION* Whiteboard::GetMapSect()
{
	return &mapSect;
}

WBClientData& Whiteboard::GetClientData(Socket pc)
{
	return clients[pc];
}

bool Whiteboard::IsCreator(const std::tstring& user) const
{
	return creator.compare(user) == 0;
}

void Whiteboard::PaintBrush(WBClientData& clientData)
{
	MouseClient mouse(clientData.mServ);
	const size_t evCount = mouse.EventCount();

	bool send = false;

	RectU rect;
	if(clientData.nVertices == 3)
		rect = RectU::Create(clientData.vertices[1], clientData.vertices[2]);
	else if(clientData.nVertices == 1 || clientData.nVertices == -1)//-1 for single line identifer
		rect = RectU::Create(clientData.vertices[0]);

	static MouseEvent prevEvent;

	for(size_t i = 0; i < evCount; i++)
	{
		const MouseEvent ev = mouse.GetEvent(i);

		const Vec2 vect = { (float)ev.GetX(), (float)ev.GetY() };

		switch(ev.GetType())
		{
		case MouseEvent::LPress:
			clientData.vertices[clientData.nVertices++] = vect;
			break;

		case MouseEvent::LRelease:
			if(clientData.nVertices == 1)
			{
				if(clientData.thickness == 1.0f)
				{
					PutPixel(clientData.vertices[0].x, clientData.vertices[0].y, clientData.clrIndex);
					rect = RectU::Create(vect);
				}
				else
				{
					const float width = clientData.thickness * 0.5f;
					const Vec2 temp = clientData.vertices[0];

					const Vec2 points[] =
					{
						ModifyPoint({ temp.x + width, temp.y + width }),
						ModifyPoint({ temp.x - width, temp.y + width }),
						ModifyPoint({ temp.x - width, temp.y - width }),
						ModifyPoint({ temp.x + width, temp.y - width })
					};

					DrawQuadrilateral(points, clientData.clrIndex);
					rect = RectU::Create(points[0], points[2]);
				}
			}

			clientData.nVertices = 0;
			send = true;
			break;

		case MouseEvent::Move:
		{
			if(clientData.nVertices != 0)
			{
				if(clientData.thickness > 1.0f)
				{
					const Vec2 norm = (vect - clientData.vertices[0]).CCW90().Normalize();
					const Vec2 normOffset = norm * clientData.thickness * 0.5f;
					Vec2 points[4];

					if(clientData.nVertices == 3)
					{
						//thickness was not changed since last update
						if(clientData.prevThickness == clientData.thickness)
						{
							points[0] = clientData.vertices[1];
							points[1] = clientData.vertices[2];
						}
						//thickness used to be 1
						else
						{
							points[0] = ModifyPoint(clientData.vertices[0] + normOffset);
							points[1] = ModifyPoint(clientData.vertices[0] - normOffset);
							rect.Modify(points[0], points[1]);
							clientData.prevThickness = clientData.thickness;
						}
					}
					else
					{
						points[0] = ModifyPoint(clientData.vertices[0] + normOffset);
						points[1] = ModifyPoint(clientData.vertices[0] - normOffset);
						rect.Modify(points[0], points[1]);
					}

					points[2] = ModifyPoint(vect - normOffset);
					points[3] = ModifyPoint(vect + normOffset);

					rect.Modify(points[2], points[3]);

					DrawQuadrilateral(points, clientData.clrIndex);

					clientData.vertices[1] = points[3];
					clientData.vertices[2] = points[2];
					clientData.nVertices = 3;
				}
				else
				{
					DrawLine(clientData.vertices[0].x, clientData.vertices[0].y, vect.x, vect.y, clientData.clrIndex);
					rect.Modify(vect);
					//-1 for single line identifer
					clientData.nVertices = -1;
				}

				clientData.vertices[0] = vect;
				send = true;
			}
			break;
		}
		}
		prevEvent = ev;
	}

	mouse.Erase(evCount);

	if(send)
		QueueSendBitmap(rect, Socket());
}

void Whiteboard::Draw()
{
	EnterCriticalSection(&mapSect);

	for(auto& it = clients.begin(), end = clients.end(); it != end; it++)
	{
		MouseClient mouse(it->second.mServ);
		const Tool myTool = it->second.tool;

		if(!mouse.MouseEmpty())
		{
			switch(myTool)
			{
			case Tool::PaintBrush:
				PaintBrush(it->second);
				break;
			}
		}
	}

	LeaveCriticalSection(&mapSect);
}

void Whiteboard::PutPixel(const PointU& point, BYTE clr)
{
	pixels[point.x + (point.y * params.width)] = clr;
}

void Whiteboard::PutPixel(int x, int y, BYTE clr)
{
	pixels[x + (y * params.width)] = clr;
}

void Whiteboard::DrawFlatTriangle(float y0, float y1, float m0, float b0, float m1, float b1, BYTE clr)
{
	const int yStart = (int)(y0 + 0.5f);
	const int yEnd = (int)(y1 + 0.5f);

	for(int y = yStart; y < yEnd; y++)
	{
		const int xStart = int(m0 * (float(y) + 0.5f) + b0 + 0.5f);
		const int xEnd = int(m1 * (float(y) + 0.5f) + b1 + 0.5f);

		for(int x = xStart; x < xEnd; x++)
		{
			PutPixel(x, y, clr);
		}
	}
}

void Whiteboard::DrawTriangle(Vec2 v0, Vec2 v1, Vec2 v2, BYTE clr)
{
	if(v1.y < v0.y) v0.Swap(v1);
	if(v2.y < v1.y) v1.Swap(v2);
	if(v1.y < v0.y) v0.Swap(v1);

	if(v0.y == v1.y)
	{
		if(v1.x < v0.x) v0.Swap(v1);
		const float m1 = (v0.x - v2.x) / (v0.y - v2.y);
		const float m2 = (v1.x - v2.x) / (v1.y - v2.y);
		float b1 = v0.x - m1 * v0.y;
		float b2 = v1.x - m2 * v1.y;
		DrawFlatTriangle(v1.y, v2.y, m1, b1, m2, b2, clr);
	}
	else if(v1.y == v2.y)
	{
		if(v2.x < v1.x) v1.Swap(v2);
		const float m0 = (v0.x - v1.x) / (v0.y - v1.y);
		const float m1 = (v0.x - v2.x) / (v0.y - v2.y);
		float b0 = v0.x - m0 * v0.y;
		float b1 = v0.x - m1 * v0.y;
		DrawFlatTriangle(v0.y, v1.y, m0, b0, m1, b1, clr);
	}
	else
	{
		const float m0 = (v0.x - v1.x) / (v0.y - v1.y);
		const float m1 = (v0.x - v2.x) / (v0.y - v2.y);
		const float m2 = (v1.x - v2.x) / (v1.y - v2.y);
		float b0 = v0.x - m0 * v0.y;
		float b1 = v0.x - m1 * v0.y;
		float b2 = v1.x - m2 * v1.y;

		const float qx = m1 * v1.y + b1;

		if(qx < v1.x)
		{
			DrawFlatTriangle(v0.y, v1.y, m1, b1, m0, b0, clr);
			DrawFlatTriangle(v1.y, v2.y, m1, b1, m2, b2, clr);
		}
		else
		{
			DrawFlatTriangle(v0.y, v1.y, m0, b0, m1, b1, clr);
			DrawFlatTriangle(v1.y, v2.y, m2, b2, m1, b1, clr);
		}
	}
}

void Whiteboard::DrawLine(int x1, int y1, int x2, int y2, BYTE clr)
{
	const int dx = x2 - x1;
	const int dy = y2 - y1;

	if(dy == 0 && dx == 0)
	{
		PutPixel(x1, y1, clr);
	}
	else if(abs(dy) > abs(dx))
	{
		if(dy < 0)
		{
			int temp = x1;
			x1 = x2;
			x2 = temp;
			temp = y1;
			y1 = y2;
			y2 = temp;
		}
		const float m = (float)dx / (float)dy;
		const float b = x1 - m*y1;
		for(int y = y1; y <= y2; y = y + 1)
		{
			const int x = (int)(m*y + b + 0.5f);
			PutPixel(x, y, clr);
		}
	}
	else
	{
		if(dx < 0)
		{
			int temp = x1;
			x1 = x2;
			x2 = temp;
			temp = y1;
			y1 = y2;
			y2 = temp;
		}
		const float m = (float)dy / (float)dx;
		const float b = y1 - m*x1;
		for(int x = x1; x <= x2; x = x + 1)
		{
			const int y = (int)(m*x + b + 0.5f);
			PutPixel(x, y, clr);
		}
	}
}

void Whiteboard::DrawQuadrilateral(const Vec2* vertices, BYTE clr)
{
	DrawTriangle(vertices[0], vertices[1], vertices[2], clr);
	DrawTriangle(vertices[0], vertices[2], vertices[3], clr);
}

Vec2 Whiteboard::ModifyPoint(const Vec2& vect)
{
	Vec2 temp = vect;

	if(temp.x < 0.0f)
		temp.x = 0.0f;

	else if(temp.x > params.width)
		temp.x = params.width;

	if(temp.y < 0.0f)
		temp.y = 0.0f;

	else if(temp.y > params.height)
		temp.y = params.height;

	return temp;
}

UINT Whiteboard::GetBufferLen(const RectU& rec) const
{
	return sizeof(RectU) + ((1 + rec.right - rec.left) * (1 + rec.bottom - rec.top));
}

void Whiteboard::MakeRectPixels(const RectU& rect, char* ptr)
{
	const USHORT offset = sizeof(RectU);
	memcpy(ptr, &rect, offset);
	ptr += offset;

	const size_t height = min(1 + rect.bottom - rect.top, params.height);
	const size_t width = min(1 + rect.right - rect.left, params.width);
	for(size_t iy = 0; iy < height; iy++)
	{
		for(size_t ix = 0; ix < width; ix++)
		{
			ptr[(iy * width) + ix] = pixels[((iy + rect.top) * params.width) + (ix + rect.left)];
		}
	}
}

std::unordered_map<Socket, WBClientData, Socket::Hash>& Whiteboard::GetMap()
{
	return clients;
}

std::vector<Socket>& Whiteboard::GetPcs()
{
	return sendPcs;
}

const WBParams& Whiteboard::GetParams() const
{
	return params;
}

void Whiteboard::AddClient(Socket pc)
{
	EnterCriticalSection(&mapSect);

	clients.emplace(pc, WBClientData(params.fps, params.clrIndex));
	sendPcs.push_back(pc);

	LeaveCriticalSection(&mapSect);
}

void Whiteboard::RemoveClient(Socket pc)
{
	EnterCriticalSection(&mapSect);

	clients.erase(pc);
	for(USHORT i = 0; i < sendPcs.size(); i++)
	{
		if(sendPcs[i] == pc)
		{
			sendPcs[i] = sendPcs.back();
			sendPcs.pop_back();
			break;
		}
	}

	LeaveCriticalSection(&mapSect);
}

const Palette& Whiteboard::GetPalette() const
{
	return palette;
}

void Whiteboard::SendBitmap(const RectU& rect)
{
	const DWORD nBytes = GetBufferLen(rect) + MSG_OFFSET ;
	char* msg = alloc<char>(nBytes);

	msg[0] = TYPE_DATA;
	msg[1] = MSG_DATA_BITMAP;

	MakeRectPixels(rect, &msg[MSG_OFFSET]);

	HANDLE hnd = serv.SendClientData(msg, nBytes, sendPcs);
	WaitAndCloseHandle(hnd);
	dealloc(msg);
}

void Whiteboard::SendBitmap(const RectU& rect, const Socket& sock, bool single)
{
	const DWORD nBytes = GetBufferLen(rect) + MSG_OFFSET;
	char* msg = alloc<char>(nBytes);

	msg[0] = TYPE_DATA;
	msg[1] = MSG_DATA_BITMAP;

	MakeRectPixels(rect, &msg[MSG_OFFSET]);

	HANDLE hnd = serv.SendClientData(msg, nBytes, sock, single);
	WaitAndCloseHandle(hnd);
	dealloc(msg);
}

void Whiteboard::QueueSendBitmap(const RectU& rect, const Socket& sock)
{	
	WaitForSingleObject(sendThread.GetWbThreadEv(), INFINITE);
	sendThread.SetRect(rect);
	sendThread.SetSinglePC(sock);
	sendThread.SingleSendThread();
	sendThread.ResetWbEv();
}

HANDLE Whiteboard::GetTimer() const
{
	return timer;
}

void Whiteboard::StartThread()
{
	LARGE_INTEGER LI;
	LI.QuadPart = (LONGLONG)(interval * -10000000.0f);
	SetWaitableTimer(timer, &LI, (LONG)(interval * 1000.0f), NULL, NULL, FALSE);

	thread = CreateThread(NULL, 0, WBThread, this, NULL, &threadID);

	sendThread.Start(SendBitmapThread);
}

Whiteboard::~Whiteboard()
{
	if(pixels)
	{
		sendThread.Exit();

		PostThreadMessage(threadID, WM_QUIT, 0, 0);
		WaitAndCloseHandle(thread);

		dealloc(pixels);

		DeleteCriticalSection(&mapSect);

		clients.clear();
		sendPcs.clear();
	}
}
