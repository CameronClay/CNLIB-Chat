#pragma once
#include "TCPServ.h"
#include "Tool.h"
#include <wincodec.h>
#include <unordered_map>
#include "..\\Common\\ColorDef.h"
#include "..\\Common\\Mouse.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "Windowscodecs.lib")

class Whiteboard
{
public:
	Whiteboard(TCPServ &serv, USHORT ScreenWidth, USHORT ScreenHeight);
	struct ClientData
	{
		Tool tool;								// Enums are sizeof(int) 4 bytes
		UINT clrIndex;							// Palette color is 4 bytes
		MouseServer mServ;						// MouseServer is 16 - 32 bytes
		std::vector<D2D1_POINT_2F> pointList;
	};

	~Whiteboard();

	void Draw(ClientData *pClientData);
	CRITICAL_SECTION & GetCritSection();
		
	
	CRITICAL_SECTION& GetMapSect();
	std::unordered_map<Socket, BYTE>& GetMap();
private:
	USHORT screenWidth, screenHeight;

	ID2D1Factory *pFactory;
	ID2D1RenderTarget *pRenderTarget;

	IWICImagingFactory *pWicFactory;
	IWICBitmap *pWicBitmap;

	CRITICAL_SECTION bitmapSect, mapSect;
	//std::unordered_map<Socket, BYTE> clients; //type should be defined class that stores all whiteboard client-specific vars
};
