#pragma once
#include "TCPServ.h"
#include <wincodec.h>
#include <unordered_map>
#include "..\\Common\\ColorDef.h"
#include "..\\Common\\WhiteboardClientData.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "Windowscodecs.lib")

class Whiteboard
{
public:
	Whiteboard(TCPServ &serv, USHORT ScreenWidth, USHORT ScreenHeight, USHORT Fps);
	Whiteboard(Whiteboard &&wb);

	~Whiteboard();

	BYTE *GetBitmap();
	void Draw(WBClientData *pClientData);
	CRITICAL_SECTION &GetCritSection();	
	CRITICAL_SECTION& GetMapSect();
	std::unordered_map<Socket, ClientData, Socket::Hash>& GetMap();
private:
	BYTE *pixels;
	USHORT screenWidth, screenHeight, fps;
	CRITICAL_SECTION bitmapSect, mapSect;
	std::unordered_map<Socket, ClientData, Socket::Hash> clients; //type should be defined class that stores all whiteboard client-specific vars

	/*
	// Won't be needed for the indexed palette, will revisit if we implement 
	// DirectWrite
	ID2D1Factory *pFactory;
	ID2D1RenderTarget *pRenderTarget;

	IWICImagingFactory *pWicFactory;
	IWICBitmap *pWicBitmap;
	*/

};
