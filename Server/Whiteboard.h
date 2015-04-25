#pragma once
#include "TCPServ.h"
#include <unordered_map>
#include "..\\Common\\ColorDef.h"
#include "..\\Common\\WhiteboardClientData.h"

//
//#include <wincodec.h>
//#pragma comment(lib, "d2d1.lib")
//#pragma comment(lib, "Windowscodecs.lib")

class Whiteboard
{
public:
	Whiteboard(TCPServ &serv, USHORT ScreenWidth, USHORT ScreenHeight, USHORT Fps, D3DCOLOR color);
	Whiteboard(Whiteboard &&wb);

	~Whiteboard();

	void Draw();
	void PaintBrush(std::deque<PointU> &pointList, BYTE clr);
	void DrawLine(PointU start, PointU end, BYTE clr);

	void MakeRect(PointU &p0, PointU &p1);
	BYTE *GetBitmap();
	CRITICAL_SECTION &GetCritSection();
	CRITICAL_SECTION& GetMapSect();
	std::unordered_map<Socket, WBClientData, Socket::Hash>& GetMap();
	std::vector<Socket>& GetPcs();
	void AddClient(Socket pc);
	void RemoveClient(Socket pc);
private:
	BYTE *pixels;
	USHORT screenWidth, screenHeight, fps;
	D3DCOLOR color;
	CRITICAL_SECTION bitmapSect, mapSect;
	std::unordered_map<Socket, WBClientData, Socket::Hash> clients; //type should be defined class that stores all whiteboard client-specific vars
	std::vector<Socket> sendPcs;
	TCPServ &serv;
	std::vector<RectU> rectList;

	/*
	// Won't be needed for the indexed palette, will revisit if we implement 
	// DirectWrite
	ID2D1Factory *pFactory;
	ID2D1RenderTarget *pRenderTarget;

	IWICImagingFactory *pWicFactory;
	IWICBitmap *pWicBitmap;
	*/

};
