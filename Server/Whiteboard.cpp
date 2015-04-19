#include "Whiteboard.h"


Whiteboard::Whiteboard(TCPServ &serv, USHORT ScreenWidth, USHORT ScreenHeight) 
	:
screenWidth(ScreenWidth),
screenHeight(ScreenHeight),
pFactory(nullptr),
pWicFactory(nullptr),
pRenderTarget(nullptr),
pWicBitmap(nullptr)
{
	HRESULT hr = CoCreateInstance(
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
		);
	InitializeCriticalSection(&cs);
}

void Whiteboard::Draw(ClientData *pClientData)
{

}

CRITICAL_SECTION &Whiteboard::GetCritSection()
{
	return cs;
}

std::unordered_map<Socket, BYTE>& Whiteboard::GetMap()
{
	return clients;
}

Whiteboard::~Whiteboard()
{
	DeleteCriticalSection(&cs);
}
