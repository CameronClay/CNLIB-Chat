#include "Whiteboard.h"


Whiteboard::Whiteboard(TCPServ &serv, USHORT ScreenWidth, USHORT ScreenHeight, USHORT Fps) 
	:
screenWidth(ScreenWidth),
screenHeight(ScreenHeight),
fps(Fps)/*,
pFactory(nullptr),
pWicFactory(nullptr),
pRenderTarget(nullptr),
pWicBitmap(nullptr)*/
{
	/*HRESULT hr = CoCreateInstance(
		CLSID_WICImagingFactory,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&pWicFactory)
		);

	pWicFactory->CreateBitmap(
		800, 
		600, 
		GUID_WICPixelFormat32bppPBGRA, 
		WICBitmapCacheOnDemand, 
		&pWicBitmap
		);

	D2D1CreateFactory(
		D2D1_FACTORY_TYPE_MULTI_THREADED, 
		&pFactory
		);

	pFactory->CreateWicBitmapRenderTarget(
		pWicBitmap, 
		D2D1::RenderTargetProperties(), 
		&pRenderTarget
		);*/
	InitializeCriticalSection(&bitmapSect);
	InitializeCriticalSection(&mapSect);
}

Whiteboard::Whiteboard(Whiteboard &&wb) 
	:
	pixels(wb.pixels),
	screenWidth(wb.screenWidth),
	fps(wb.fps),
	bitmapSect(wb.bitmapSect),
	mapSect(wb.mapSect),
	clients(wb.clients)
{
	ZeroMemory(&wb, sizeof(Whiteboard));
}

BYTE *Whiteboard::GetBitmap()
{
	EnterCriticalSection(&bitmapSect);
	return nullptr;
	LeaveCriticalSection(&bitmapSect);
}

CRITICAL_SECTION& Whiteboard::GetMapSect()
{
	return mapSect;
}

std::unordered_map<Socket, Whiteboard::ClientData, Socket::Hash>& Whiteboard::GetMap()
{
	return clients;
}

Whiteboard::~Whiteboard()
{
	//need a way to check if has been inited for mctor
	//if()
	{
		DeleteCriticalSection(&bitmapSect);
	}
	//if()
	{
		DeleteCriticalSection(&mapSect);
	}
}
