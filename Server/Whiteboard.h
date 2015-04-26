#pragma once
#include "TCPServ.h"
#include <unordered_map>
#include "..\\Common\\ColorDef.h"
#include "..\\Common\\WhiteboardClientData.h"

class Whiteboard
{
public:
	Whiteboard(TCPServ &serv, WBParams params);
	Whiteboard(Whiteboard &&wb);

	~Whiteboard();

	void Draw();
	void PaintBrush(std::deque<PointU> &pointList, BYTE clr);
	void DrawLine(PointU start, PointU end, BYTE clr);

	void MakeRect(PointU &p0, PointU &p1);
	BYTE *GetBitmap();
	void InitPalette();
	CRITICAL_SECTION &GetBitmapSection();
	CRITICAL_SECTION& GetMapSect();
	std::unordered_map<Socket, WBClientData, Socket::Hash>& GetMap();
	std::vector<Socket>& GetPcs();
	void AddClient(Socket pc);
	void RemoveClient(Socket pc);
private:
	USHORT screenWidth, screenHeight, fps;
	BYTE *pixels;
	BYTE bgColor;
	BYTE color;
	CRITICAL_SECTION bitmapSect, mapSect;
	std::vector<Socket> sendPcs;
	TCPServ &serv;
	D3DCOLOR palette[32];

	std::unordered_map<Socket, WBClientData, Socket::Hash> clients;
	std::vector<RectU> rectList;

};
