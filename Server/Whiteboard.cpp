#include "Whiteboard.h"
#include "CNLIB\HeapAlloc.h"
#include "CNLIB\Messages.h"

Whiteboard::Whiteboard(TCPServInterface &serv, WBParams params, std::tstring creator)
	:
params(std::move(params)),
serv(serv),
creator(creator),
interval(1.0f / (float)params.fps)
{
	pixels = alloc<BYTE>(params.width * params.height);
	FillMemory(pixels, params.width * params.height, params.clrIndex);

	InitializeCriticalSection(&bitmapSect);
	InitializeCriticalSection(&mapSect);
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

void Whiteboard::PaintBrush(MouseClient& mouse, BYTE clr)
{
	MouseEvent ev;
	while(mouse.Peek().GetType() != MouseEvent::Type::Invalid)
	{
		ev = mouse.Read();

		PointU pt;
		pt.x = ev.GetX();
		pt.y = ev.GetY();

		switch(ev.GetType())
		{
		case MouseEvent::LRelease:
			pointList.pop_back();
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

		if(pointList.size() == 2)
		{
			const RectU rect = MakeRect(pointList[0], pointList[1]);

			DrawLine(rect, clr);

			pointList.pop_front();

			SendBitmap(rect);
		}
	}
}

void Whiteboard::Draw()
{
	EnterCriticalSection(&bitmapSect);
	EnterCriticalSection(&mapSect);

	for (auto& it : clients)
	{
		MouseClient mouse(it.second.mServ);
		const Tool myTool = it.second.tool;
		const BYTE color = 12;

		if(!mouse.MouseEmpty())
		{
			switch(myTool)
			{
			case Tool::PaintBrush:
				PaintBrush(mouse, color);
				break;
			}
		}
	}

	LeaveCriticalSection(&mapSect);
	LeaveCriticalSection(&bitmapSect);
}

void Whiteboard::DrawLine(const RectU& rect, BYTE clr)
{
	PointU dist(rect.right - rect.left, rect.bottom - rect.top);
	float len = dist.Length();
	dist = dist.Normalize();

	for (float i = 0.0f; i < len; i++)
	{
		PointU pos = (dist * i);
		pos.x += rect.left;
		pos.y += rect.top;

		UINT index = (pos.y * params.width) + pos.x;
		pixels[index] = clr;
	}
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

	const USHORT height = rect.bottom - rect.top, 
		width = rect.right - rect.left;

	for(USHORT iy = 0; iy < height; iy++)
	{
		for(USHORT ix = 0; ix < width; ix++)
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
