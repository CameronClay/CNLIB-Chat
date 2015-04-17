#pragma once
#include "Tool.h"
#include "TCPServ.h"
#include <d2d1.h>
#include <wincodec.h>
#include <unordered_map>
#include "..\\Client\\TCPClient.h"
#include "..\\Common\\ColorDef.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "Windowscodecs.lib")

class Whiteboard
{
public:
	Whiteboard(TCPServ &serv, UINT ScreenWidth, UINT ScreenHeight);
	~Whiteboard();

	void Draw(Tool *pTool)
	{
		pTool->Draw();
	}
	void GetCritSection(CRITICAL_SECTION &cs);
	void GetUMap(std::unordered_map<Socket, TCPClient> &um);
	void GetBitmap();
private:
	UINT screenWidth, screenHeight;

	ID2D1Factory *pFactory;
	ID2D1RenderTarget *pRenderTarget;

	IWICImagingFactory *pWicFactory;
	IWICBitmap *pWicBitmap;

	CRITICAL_SECTION cs;
	std::unordered_map<Socket, TCPClient> clients;
};
