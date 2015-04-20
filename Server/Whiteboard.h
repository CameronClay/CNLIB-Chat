#pragma once
#include "TCPServ.h"
#include "Tool.h"
#include <wincodec.h>
#include <unordered_map>
#include "ColorDef.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "Windowscodecs.lib")

class Whiteboard
{
public:
	Whiteboard(TCPServ &serv, USHORT ScreenWidth, USHORT ScreenHeight);
	Whiteboard(Whiteboard&& wb);
	~Whiteboard();

	void Draw(Tool *pTool)
	{
		pTool->Draw();
	}
	CRITICAL_SECTION& GetMapSect();
	std::unordered_map<Socket, BYTE>& GetMap();
	void GetBitmap();
private:
	USHORT screenWidth, screenHeight;

	ID2D1Factory *pFactory;
	ID2D1RenderTarget *pRenderTarget;

	IWICImagingFactory *pWicFactory;
	IWICBitmap *pWicBitmap;

	CRITICAL_SECTION bitmapSect, mapSect;
	//std::unordered_map<Socket, BYTE> clients; //type should be defined class that stores all whiteboard client-specific vars
};
