#include "Whiteboard.h"


Whiteboard::Whiteboard(TCPServ &serv, UINT ScreenWidth, UINT ScreenHeight) :
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

void Whiteboard::GetBitmap()
{
	EnterCriticalSection(&cs);
	UINT width = 0, height = 0;
	HRESULT hr = pWicBitmap->GetSize(&width, &height);
	if (SUCCEEDED(hr))
	{
		//pWicFactory->CreateBitmapFromSource(pWicBitmap, WICBitmapCacheOnDemand, ppBitmap);
		
	}
	LeaveCriticalSection(&cs);
}

void Whiteboard::GetCritSection(CRITICAL_SECTION &critsect)
{
	critsect = cs;
}

void Whiteboard::GetUMap(std::unordered_map<Socket, TCPClient> &um)
{
	EnterCriticalSection(&cs);
	um = clients;
	LeaveCriticalSection(&cs);
}

Whiteboard::~Whiteboard()
{
	DeleteCriticalSection(&cs);
}
