#include "Whiteboard.h"
#include "HeapAlloc.h"

Whiteboard::Whiteboard(TCPServ &serv, WBParams params, std::tstring creator)
	:
params(std::forward<WBParams>(params)),
serv(serv),
creator(creator)
{
	pixels = alloc<BYTE>(params.width * params.height);
	FillMemory(pixels, params.width * params.height, params.clrIndex);

	InitializeCriticalSection(&bitmapSect);
	InitializeCriticalSection(&mapSect);
}

Whiteboard::Whiteboard(Whiteboard &&wb) :
params(std::forward<WBParams>(wb.params)),
pixels(wb.pixels),
bitmapSect(wb.bitmapSect),
mapSect(wb.mapSect),
serv(wb.serv),
creator(wb.creator),
clients(wb.clients),
sendPcs(wb.sendPcs),
rectList(wb.rectList)
{
	wb.pixels = nullptr;
}

BYTE *Whiteboard::GetBitmap()
{
	// LeaveCriticalSection will never be executed in this case, probably could 
	// use a wrapper for something like this where Enter in ctor and Leave in dtor
	/*
	struct CritSectionGuard
	{
		CritSectionGuard( CRITICAL_SECTION *pCritSect ) :
		pCriticalSection(pCritSect)
		{
			EnterCriticalSection(pCritSect);
		}
		~CritSectionGuard()
		{
			LeaveCriticalSection(pCritSect);
		}

		CRITICAL_SECTION *pCriticalSection;
	*/
	EnterCriticalSection(&bitmapSect);
	return pixels;
	LeaveCriticalSection(&bitmapSect);
}

CRITICAL_SECTION& Whiteboard::GetMapSect()
{
	return mapSect;
}

CRITICAL_SECTION &Whiteboard::GetBitmapSection()
{
	return bitmapSect;
}

bool Whiteboard::IsCreator(std::tstring& user) const
{
	return creator.compare(user) == 0;
}

void Whiteboard::PaintBrush(std::deque<PointU> &pointList, BYTE clr)
{
	for (int i = 0; i < pointList.size(); i++)
	{
		if (pointList.size() >= 2)
		{
			MakeRect(pointList[0], pointList[1]);

			DrawLine(pointList[0], pointList[1], clr);

			pointList.pop_front();
		}
		else if (pointList.size() == 1)
		{
			PointU p0, p1;

			p0.x = pointList[0].x - 1;
			p0.y = pointList[0].y - 1;
			p1.x = pointList[0].x + 1;
			p1.y = pointList[0].y + 1;

			MakeRect(p0, p1);

			DrawLine(p0, p1, clr);

			pointList.pop_front();
		}
	}
}

void Whiteboard::Draw()
{
	EnterCriticalSection(&mapSect);
	for (auto it : clients)
	{		
		MouseClient mouse(it.second.mServ);
		Tool myTool = it.second.tool;
		BYTE color = it.second.clrIndex;
		
		std::deque<PointU> pointList;
		while (mouse.Read().GetType() != MouseEvent::Type::Invalid)
		{
			PointU pt;
			pt.x = mouse.GetX();
			pt.y = mouse.GetY();
			pointList.push_back(pt);
		}

		switch (myTool)
		{
		case Tool::PaintBrush:
			PaintBrush(pointList, color);
			break;
		}

		pointList.clear();
	}

	LeaveCriticalSection(&mapSect);
}

void Whiteboard::DrawLine(PointU start, PointU end, BYTE clr)
{
	PointU dist = end - start;
	float len = dist.Length();
	dist = dist.Normalize();	

	for (float i = 0.0f; i < len; i++)
	{
		PointU pos = (dist * i) + start;

		int index = (pos.y * params.width) + pos.x;
		pixels[index] = clr;
	}
}

void Whiteboard::MakeRect(PointU &p0, PointU &p1)
{
	RectU rect;

	p0.x < p1.x ?
		rect.left = p0.x, rect.right = p1.x :
		rect.left = p1.x, rect.right = p0.x;

	p0.y < p1.y ?
		rect.top = p0.y, rect.bottom = p1.y :
		rect.top = p1.y, rect.bottom = p0.y;

	rectList.push_back(rect);
}

std::unordered_map<Socket, WBClientData, Socket::Hash>& Whiteboard::GetMap()
{
	return clients;
}

std::vector<Socket>& Whiteboard::GetPcs()
{
	return sendPcs;
}

WBParams& Whiteboard::GetParams()
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

	clients.emplace(pc, WBClientData());
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

const Palette &Whiteboard::GetPalette() const
{
	return palette;
}

Whiteboard::~Whiteboard()
{
	//need a way to check if has been inited for mctor
	if(pixels)
	{
		DeleteCriticalSection(&bitmapSect);
		DeleteCriticalSection(&mapSect);
		clients.clear();
		sendPcs.clear();
		dealloc(pixels);
	}
}
