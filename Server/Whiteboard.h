#pragma once
#include "TCPServ.h"
#include <unordered_map>
#include "..\\Common\\ColorDef.h"
#include "..\\Common\\WhiteboardClientData.h"

class Whiteboard
{
public:
	Whiteboard(TCPServ &serv, WBParams params, std::tstring creator);
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
	bool IsCreator(std::tstring& user) const;
	void AddClient(Socket pc);
	void RemoveClient(Socket pc);
	WBParams& GetParams();
private:
	WBParams params;
	BYTE *pixels;
	CRITICAL_SECTION bitmapSect, mapSect;
	TCPServ &serv;
	D3DCOLOR palette[32];
	std::tstring creator;

	std::unordered_map<Socket, WBClientData, Socket::Hash> clients;
	std::vector<Socket> sendPcs;
	std::vector<RectU> rectList;
};
