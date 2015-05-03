#pragma once
#include "TCPServ.h"
#include <unordered_map>
#include "ColorDef.h"
#include "WhiteboardClientData.h"
#include "Palette.h"
#include "Timer.h"

class Whiteboard
{
public:
	Whiteboard(TCPServ &serv, WBParams params, std::tstring creator);
	Whiteboard(Whiteboard &&wb);

	~Whiteboard();

	void Draw();
	void SendBitmap();
	void SendBitmap(RectU& rect, Socket& sock, bool single);
	void Frame();

	BYTE* GetBitmap();
	CRITICAL_SECTION& GetBitmapSection();
	CRITICAL_SECTION& GetMapSect();
	std::unordered_map<Socket, WBClientData, Socket::Hash>& GetMap();
	std::vector<Socket>& GetPcs();
	bool IsCreator(std::tstring& user) const;
	void AddClient(Socket pc);
	void RemoveClient(Socket pc);
	const WBParams& GetParams() const;
	const Palette& GetPalette() const;

private:
	void PaintBrush(std::deque<PointU> &pointList, BYTE clr);
	void DrawLine(PointU start, PointU end, BYTE clr);
	void MakeRect(PointU &p0, PointU &p1);
	UINT GetBufferLen() const;
	void MakeRectPixels(RectU& rect, char* ptr);

private:
	WBParams params;
	BYTE *pixels; 
	CRITICAL_SECTION bitmapSect, mapSect;
	TCPServ &serv;
	Palette palette;
	std::tstring creator;

	const float interval;
	Timer timer;

	std::unordered_map<Socket, WBClientData, Socket::Hash> clients;
	std::vector<Socket> sendPcs;
	std::queue<RectU> rectList;
};
