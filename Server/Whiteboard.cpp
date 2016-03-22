#include "StdAfx.h"
#include "Whiteboard.h"
#include "CNLIB\Messages.h"
#include "MessagesExt.h"

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

Whiteboard::Whiteboard(TCPServInterface &serv, WBParams params, std::tstring creator)
	:
	pixels(alloc<BYTE>(params.width * params.height)),
	params(std::move(params)),
	serv(serv),
	creator(creator),
	interval(1.0f / (float)params.fps),
	timer(CreateWaitableTimer(NULL, FALSE, NULL)),
	thread(NULL),
	threadID(NULL)
{
	FillMemory(pixels, params.width * params.height, params.clrIndex);

	InitializeCriticalSection(&mapSect);
	srand((unsigned)time(NULL));
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
	clients(std::move(wb.clients)),
	sendPcs(std::move(wb.sendPcs))
{
	wb.pixels = nullptr;
}


void Whiteboard::SendBitmap(RectU& rect)
{
	const DWORD nBytes = GetBufferLen(rect) + MSG_OFFSET;
	BuffSendInfo sendInfo = serv.GetSendBuffer(nBytes);

	((short*)sendInfo.buffer)[0] = TYPE_DATA;
	((short*)sendInfo.buffer)[1] = MSG_DATA_BITMAP;

	MakeRectPixels(rect, &sendInfo.buffer[MSG_OFFSET]);

	serv.SendClientData(sendInfo, nBytes, sendPcs);
}

void Whiteboard::SendBitmap(RectU& rect, ClientData* clint, bool single)
{
	const DWORD nBytes = GetBufferLen(rect) + MSG_OFFSET;
	BuffSendInfo sendInfo = serv.GetSendBuffer(nBytes);

	((short*)sendInfo.buffer)[0] = TYPE_DATA;
	((short*)sendInfo.buffer)[1] = MSG_DATA_BITMAP;

	MakeRectPixels(rect, &sendInfo.buffer[MSG_OFFSET]);

	serv.SendClientData(sendInfo, nBytes, clint, single);
}

void Whiteboard::StartThread()
{
	LARGE_INTEGER LI;
	LI.QuadPart = (LONGLONG)(interval * -10000000.0f);
	SetWaitableTimer(timer, &LI, (LONG)(interval * 1000.0f), NULL, NULL, FALSE);

	thread = CreateThread(NULL, 0, WBThread, this, NULL, &threadID);
}


bool Whiteboard::IsCreator(const std::tstring& user) const
{
	return creator.compare(user) == 0;
}


void Whiteboard::AddClient(ClientData* clint)
{
	EnterCriticalSection(&mapSect);

	clients.emplace(clint, WBClientData(params.fps, params.clrIndex));
	sendPcs.push_back(clint);

	LeaveCriticalSection(&mapSect);
}

void Whiteboard::RemoveClient(ClientData* clint)
{
	EnterCriticalSection(&mapSect);

	clients.erase(clint);
	for (USHORT i = 0, size = sendPcs.size(); i < size; i++)
	{
		if (sendPcs[i] == clint)
		{
			sendPcs[i] = sendPcs.back();
			sendPcs.pop_back();
			break;
		}
	}

	LeaveCriticalSection(&mapSect);
}


void Whiteboard::PaintBrush(WBClientData& clientData)
{
	MouseClient mouse(clientData.mServ);
	const size_t evCount = mouse.EventCount();

	RectU rect;
	if(clientData.nVertices == 3)
		rect = RectU::Create(clientData.vertices[1], clientData.vertices[2]);
	else if(clientData.nVertices == 1 || clientData.nVertices == -1)//-1 for single line identifer
		rect = RectU::Create(clientData.vertices[0]);

	bool send = false;

	for(size_t i = 0; i < evCount; i++)
	{
		const MouseEvent ev = mouse.GetEvent(i);
		const Vec2 vect = { (float)ev.GetX(), (float)ev.GetY() };

		switch(ev.GetType())
		{
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
					DrawLine((int)clientData.vertices[0].x, (int)clientData.vertices[0].y, (int)vect.x, (int)vect.y, clientData.clrIndex);
					rect.Modify(vect);
					//-1 for single line identifer
					clientData.nVertices = -1;
				}
				clientData.vertices[0] = vect;
				send = true;
			}
			break;
		}
		case MouseEvent::LPress:
			clientData.vertices[clientData.nVertices++] = vect;
			break;
		case MouseEvent::LRelease:
			if(clientData.nVertices == 1)
			{
				if(clientData.thickness == 1.0f)
				{
					PutPixel((int)clientData.vertices[0].x, (int)clientData.vertices[0].y, clientData.clrIndex);
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
		}
	}

	mouse.Erase(evCount);

	if (send)
		SendBitmap(rect);
}

void Whiteboard::Draw()
{
	EnterCriticalSection(&mapSect);

	for(auto it = clients.begin(), end = clients.end(); it != end; it++)
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

Vec2 Whiteboard::ModifyPoint(Vec2 vect)
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

void Whiteboard::MakeRectPixels(RectU& rect, char* ptr)
{
	if(rect.bottom + 1 < params.height)
		rect.bottom += 1;
	if(rect.right + 1 < params.width)
		rect.right += 1;

	const USHORT offset = sizeof(RectU);
	memcpy(ptr, &rect, offset);
	ptr += offset;

	const size_t height = rect.bottom - rect.top;
	const size_t width = rect.right - rect.left;
	for(size_t iy = 0; iy < height; iy++)
	{
		for(size_t ix = 0; ix < width; ix++)
		{
			ptr[(iy * width) + ix] = pixels[((iy + rect.top) * params.width) + (ix + rect.left)];
		}
	}
}


std::unordered_map<ClientData*, WBClientData>& Whiteboard::GetMap()
{
	return clients;
}

std::vector<ClientData*>& Whiteboard::GetPcs()
{
	return sendPcs;
}

const WBParams& Whiteboard::GetParams() const
{
	return params;
}

const Palette& Whiteboard::GetPalette() const
{
	return palette;
}

HANDLE Whiteboard::GetTimer() const
{
	return timer;
}

BYTE* Whiteboard::GetBitmap()
{
	return pixels;
}

CRITICAL_SECTION* Whiteboard::GetMapSect()
{
	return &mapSect;
}

WBClientData& Whiteboard::GetClientData(ClientData* pc)
{
	return clients[pc];
}


Whiteboard::~Whiteboard()
{
	if(pixels)
	{
		PostThreadMessage(threadID, WM_QUIT, 0, 0);
		WaitAndCloseHandle(thread);

		dealloc(pixels);

		DeleteCriticalSection(&mapSect);

		clients.clear();
		sendPcs.clear();
	}
}
