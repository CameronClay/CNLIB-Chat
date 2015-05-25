#include "Whiteboard.h"
#include "CNLIB\HeapAlloc.h"
#include "CNLIB\Messages.h"
#include <stdlib.h>
#include <time.h>

Whiteboard::Whiteboard(TCPServInterface &serv, WBParams params, std::tstring creator)
	:
params(std::move(params)),
serv(serv),
creator(creator),
interval(1000.0f / (float)params.fps)
{
	pixels = alloc<BYTE>(params.width * params.height);
	FillMemory(pixels, params.width * params.height, params.clrIndex);

	InitializeCriticalSection(&bitmapSect);
	InitializeCriticalSection(&mapSect);
	srand(time(NULL));
}

Whiteboard::Whiteboard(Whiteboard &&wb)
	:
params(std::move(wb.params)),
pixels(wb.pixels),
bitmapSect(wb.bitmapSect),
mapSect(wb.mapSect),
serv(std::move(wb.serv)),
creator(std::move(wb.creator)),
interval(wb.interval),
timer(wb.timer),
clients(std::move(wb.clients)),
sendPcs(std::move(wb.sendPcs)),
pointList(std::move(wb.pointList))
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

CRITICAL_SECTION* Whiteboard::GetBitmapSection()
{
	return &bitmapSect;
}

bool Whiteboard::IsCreator(const std::tstring& user) const
{
	return creator.compare(user) == 0;
}

void Whiteboard::PaintBrush(WBClientData& clientData, BYTE clr)
{
	RectU& rect = clientData.rect;
	MouseClient mouse(clientData.mServ);
	MouseEvent ev = mouse.Peek();

	if(ev.GetType() != MouseEvent::Type::Invalid)
	{
		ev = mouse.Read();
		bool send = false;

		PointU pt(ev.GetX(), ev.GetY());
		if(!pointList.empty())
		{
			rect.left = (pointList[0].x > 0 ? pointList[0].x - 1 : 0);
			rect.right = (pointList[0].x < params.width ? pointList[0].x + 1 : params.width);
			rect.top = (pointList[0].y > 0 ? pointList[0].y - 1 : 0);
			rect.bottom = (pointList[0].y < params.height ? pointList[0].y + 1 : params.height);
		}
		else
		{
			rect.left = (pt.x > 0 ? pt.x - 1 : 0);
			rect.right = (pt.x < params.width ? pt.x + 1 : params.width);
			rect.top = (pt.y > 0 ? pt.y - 1 : 0);
			rect.bottom = (pt.y < params.height ? pt.y + 1 : params.height);
		}

		do
		{
			switch(ev.GetType())
			{
			case MouseEvent::LRelease:
				PutPixel(pointList[0], clr);
				pointList.pop_back();
				send = true;
				break;
			case MouseEvent::LPress:
				if(pointList.empty())
					pointList.push_back(pt);
				break;
			case MouseEvent::Move:
				if(pointList.size() == 1)
					pointList.push_back(pt);
				break;
			}

			if(pointList.size() >= 2)
			{
				//DrawLineThick(pointList[0], pointList[1], 15.0f, clr);
				DrawLine(pointList[0], pointList[1], clr);
				pointList.pop_front();
				send = true;
			}

			if(pt.x - 1 < rect.left)
				rect.left = (pt.x > 0 ? pt.x - 1 : 0);
			else if(pt.x + 1 > rect.right)
				rect.right = (pt.x < params.width ? pt.x + 1 : params.width);

			if(pt.y - 1 < rect.top)
				rect.top = (pt.y > 0 ? pt.y - 1 : 0);
			else if(pt.y + 1 > rect.bottom)
				rect.bottom = (pt.y < params.height ? pt.y + 1 : params.height);

			ev = mouse.Read();

		} while(ev.GetType() != MouseEvent::Type::Invalid);

		if(send)
			SendBitmap(rect);
	}
}

void Whiteboard::Draw()
{
	EnterCriticalSection(&mapSect);

	for (auto& it : clients)
	{
		MouseClient mouse(it.second.mServ);
		const Tool myTool = it.second.tool;

		if(!mouse.MouseEmpty())
		{
			BYTE color = 255;
			do
			{
				color = rand() % 31;
			} while(color == params.clrIndex);

			EnterCriticalSection(&bitmapSect);
			switch(myTool)
			{
			case Tool::PaintBrush:
				PaintBrush(it.second, color);
				break;
			}
			LeaveCriticalSection(&bitmapSect);
		}
	}

	LeaveCriticalSection(&mapSect);
}

void Whiteboard::PutPixel(const PointU& point, BYTE clr)
{
	pixels[point.x + (point.y * params.width)] = clr;
}

void Whiteboard::PutPixel(USHORT x, USHORT y, BYTE clr)
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

void Whiteboard::DrawLine(const PointU& p1, const PointU& p2, BYTE clr)
{
	short x1 = p1.x, x2 = p2.x, y1 = p1.y, y2 = p2.y;
	const short dx = x2 - x1;
	const short dy = y2 - y1;

	if(dy == 0 && dx == 0)
	{
		PutPixel(x1, y1, clr);
	}
	else if(abs(dy) > abs(dx))
	{
		if(dy < 0)
		{
			short temp = x1;
			x1 = x2;
			x2 = temp;
			temp = y1;
			y1 = y2;
			y2 = temp;
		}
		const float m = (float)dx / (float)dy;
		const float b = x1 - m*y1;
		for(short y = y1; y <= y2; y = y + 1)
		{
			const short x = (short)(m*y + b + 0.5f);
			PutPixel(x, y, clr);
		}
	}
	else
	{
		if(dx < 0)
		{
			short temp = x1;
			x1 = x2;
			x2 = temp;
			temp = y1;
			y1 = y2;
			y2 = temp;
		}
		const float m = (float)dy / (float)dx;
		const float b = y1 - m*x1;
		for(short x = x1; x <= x2; x = x + 1)
		{
			const short y = (short)(m*x + b + 0.5f);
			PutPixel(x, y, clr);
		}
	}
}

void Whiteboard::DrawLineThick(const PointU& p1, const PointU& p2, float width, BYTE clr)
{
	const Vec2 v1(p1.x, p1.y), v2(p2.x, p2.y);
	const Vec2 norm = (v2 - v1).CCW90().Normalize();
	const Vec2 normOffset = norm * width * 0.5f;
	const Vec2 perpOffset = { norm.x, - norm.y };

	const Vec2 points[] =
	{
		v1 - normOffset,
		v1 + perpOffset,
		v2 + normOffset,
		v2 - perpOffset
	};

	DrawTriangle(points[0], points[1], points[2], clr);
	DrawTriangle(points[0], points[2], points[3], clr);
}

RectU Whiteboard::MakeRect(const PointU &p0, const PointU &p1)
{
	RectU rect;

	if(p0.x < p1.x)
		rect.left = p0.x, rect.right = p1.x;
	else if(p0.x > p1.x)
		rect.left = p1.x, rect.right = p0.x;
	else
		rect.left = p0.x - 1, rect.right = p1.x + 1;

	if(p0.y < p1.y)
		rect.top = p0.y, rect.bottom = p1.y;
	else if(p0.y > p1.y)
		rect.top = p1.y, rect.bottom = p0.y;
	else
		rect.top = p0.y - 1, rect.bottom = p1.y + 1;

	//Not working right
	/*p0.x < p1.x ?
		rect.left = p0.x, rect.right = p1.x :
		rect.left = p1.x, rect.right = p0.x;

	p0.y < p1.y ?
		rect.top = p0.y, rect.bottom = p1.y :
		rect.top = p1.y, rect.bottom = p0.y;*/

	return rect;
}

void Whiteboard::Frame()
{
	if(timer.GetTimeMilli() >= interval)
	{
		Draw();
		timer.Reset();
	}
}

UINT Whiteboard::GetBufferLen(const RectU& rec) const
{
	return sizeof(RectU) + ((rec.right - rec.left) * (rec.bottom - rec.top));
}

void Whiteboard::MakeRectPixels(const RectU& rect, char* ptr)
{
	const USHORT offset = sizeof(RectU);
	memcpy(ptr, &rect, offset);
	ptr += offset;

	for(USHORT iy = 0, height = rect.bottom - rect.top; iy < height; iy++)
	{
		for(USHORT ix = 0, width = rect.right - rect.left; ix < width; ix++)
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

	clients.emplace(pc, WBClientData());
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
	const DWORD nBytes = GetBufferLen(rect) + MSG_OFFSET;
	char* msg = alloc<char>(nBytes);
	msg[0] = TYPE_DATA;
	msg[1] = MSG_DATA_BITMAP;

	MakeRectPixels(rect, &msg[MSG_OFFSET]);

	HANDLE hnd = serv.SendClientData(msg, nBytes, sendPcs);
	WaitAndCloseHandle(hnd);
	dealloc(msg);
}

void Whiteboard::SendBitmap(const RectU& rect, Socket& sock, bool single)
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

Whiteboard::~Whiteboard()
{
	//need a way to check if has been inited for mctor
	if(pixels)
	{
		dealloc(pixels);

		DeleteCriticalSection(&bitmapSect);
		DeleteCriticalSection(&mapSect);

		pointList.clear();
		clients.clear();
		sendPcs.clear();
	}
}
