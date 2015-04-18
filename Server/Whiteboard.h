#pragma once
#include "Tool.h"
#include "TCPServ.h"
#include <d2d1.h>
#include <wincodec.h>
#include <unordered_map>
#include "ColorDef.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "Windowscodecs.lib")

class Whiteboard
{
public:
	Whiteboard(TCPServ &serv, USHORT ScreenWidth, USHORT ScreenHeight);
	~Whiteboard();

	void Draw(Tool *pTool)
	{
		pTool->Draw();
	}
	CRITICAL_SECTION& GetCritSection();
	std::unordered_map<Socket, BYTE>& GetMap();
	void GetBitmap();
private:
	USHORT screenWidth, screenHeight;

	ID2D1Factory *pFactory;
	ID2D1RenderTarget *pRenderTarget;

	IWICImagingFactory *pWicFactory;
	IWICBitmap *pWicBitmap;

	CRITICAL_SECTION cs;
	std::unordered_map<Socket, BYTE> clients;
};
