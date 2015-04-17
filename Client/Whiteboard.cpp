#include "Whiteboard.h"
#include <assert.h>

Whiteboard::Whiteboard()
{
}

Whiteboard::Whiteboard(HWND WinHandle, UINT Width, UINT Height, UINT FPS, UINT palIndex) :
hWnd(WinHandle),
width(Width),
height(Height),
interval(1000 / FPS)/*,
bp(D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED))),
uRect(D2D1::RectU(0, 0, Width, Height))*/
{
	InitD3D();
	InitPalette();
	bgColor = palette[palIndex];
	pDevice->Clear(0, NULL, D3DCLEAR_TARGET, bgColor, 0.0f, 0);
	/*hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &p2DFactory);
	assert(SUCCEEDED(hr));

	hr = p2DFactory->CreateHwndRenderTarget(
		D2D1::RenderTargetProperties(), 
		D2D1::HwndRenderTargetProperties(hWnd), 
		&pHwndRenderTarget);

	D2D1_SIZE_U bmpSize = D2D1::SizeU(Width, Height);
	hr = pHwndRenderTarget->CreateBitmap(bmpSize, bp, &pBitmap);*/
}

void Whiteboard::Frame(RECT &rect, BYTE *pixelData)
{
	if (timer.GetTimeMilli() >= interval)
	{
		BeginFrame();
		Render(rect, pixelData);
		EndFrame();

		timer.Reset();
	}
}

void Whiteboard::BeginFrame()
{
	HRESULT hr = 0;
	hr = pBackBuffer->LockRect(&lockRect, nullptr, NULL);
	assert(SUCCEEDED(hr));

	ClearTempSurface();
}

void Whiteboard::Render(RECT &rect, BYTE *pixelData)
{
	UINT width = rect.right - rect.left;
	UINT height = rect.bottom - rect.top;

	ComposeImage(width, height, pixelData);

	for (int y = 0; y < height; y++)
	{
		// Pitch won't always be the same as width * 4
		UINT bRowOffset = (y * pitch) + rect.top;
		UINT rectRowOffset = y * width;

		for (int x = 0; x < pitch; x++)
		{
			UINT bIndex = x + bRowOffset + rect.left;
			UINT rIndex = x + rectRowOffset;
			D3DCOLOR *pSurface = (D3DCOLOR*)(lockRect.pBits);
			pSurface[bIndex] = tempSurface[rIndex];
		}
	}

	/*UINT pitch = Width * 4;
	
	hr = pBitmap->CopyFromMemory(&uRect, pixelData, pitch);
	assert(SUCCEEDED(hr));

	pHwndRenderTarget->BeginDraw();

	D2D1_RECT_F rect = D2D1::RectF(0.0f, 0.0f, Width, Height);
	pHwndRenderTarget->DrawBitmap(pBitmap, rect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
		rect);

	hr = pHwndRenderTarget->EndDraw();
	assert(SUCCEEDED(hr));*/
}

void Whiteboard::EndFrame()
{
	HRESULT hr = 0;
	hr = pBackBuffer->UnlockRect();
	assert(SUCCEEDED(hr));

	hr = pDevice->Present(nullptr, nullptr, nullptr, nullptr);
	assert(SUCCEEDED(hr));
}

void Whiteboard::ComposeImage(UINT Width, UINT Height, BYTE *pixelData)
{
	HRESULT hr = 0;
	tempSurface = new D3DCOLOR[Width * Height];

	for (int y = 0; y< height; y++)
	{
		UINT rowOffset = y * Width;
		for (int x = 0; x < Width; x++)
		{
			UINT index = x * rowOffset;
			UINT paletteIndex = pixelData[index];

			tempSurface[index] = palette[paletteIndex];
		}
	}
}

void Whiteboard::ClearTempSurface()
{
	if (tempSurface)
	{
		delete[] tempSurface;
		tempSurface = nullptr;
	}
}

void Whiteboard::InitD3D()
{
	HRESULT hr = 0;

	lockRect.pBits = NULL;
	pDirect3D = Direct3DCreate9(D3D_SDK_VERSION);

	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	d3dpp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

	hr = pDirect3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE, &d3dpp, &pDevice);
	if (FAILED(hr))
	{
		hr = pDirect3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
			D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE, &d3dpp, &pDevice);
	}
	assert(SUCCEEDED(hr));

	hr = pDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
	assert(SUCCEEDED(hr));

	// D3DLOCKED_RECT::Pitch is in bytes, 
	// divide by 4 (>> 2) to cnvert to D3DCOLOR (sizeof(int))
	// Put in ctor since pitch won't change, unless run on laptop and they dock
	// while running client
	hr = pBackBuffer->LockRect(&lockRect, nullptr, NULL);
	assert(SUCCEEDED(hr));

	pitch = lockRect.Pitch >> 2;

	hr = pBackBuffer->UnlockRect();
	assert(SUCCEEDED(hr));
}

void Whiteboard::InitPalette()
{
	UINT i = 0;
	palette[i++] = Black;
	palette[i++] = DarkGray;
	palette[i++] = LightGray;
	palette[i++] = White;
	palette[i++] = DarkRed;
	palette[i++] = MediumRed;
	palette[i++] = Red;
	palette[i++] = LightRed;
	palette[i++] = DarkOrange;
	palette[i++] = MediumOrange;
	palette[i++] = Orange;
	palette[i++] = LightOrange;
	palette[i++] = DarkYellow;
	palette[i++] = MediumYellow;
	palette[i++] = Yellow;
	palette[i++] = LightYellow;
	palette[i++] = DarkGreen;
	palette[i++] = MediumGreen;
	palette[i++] = Green;
	palette[i++] = LightGreen;
	palette[i++] = DarkCyan;
	palette[i++] = MediumCyan;
	palette[i++] = Cyan;
	palette[i++] = LightCyan;
	palette[i++] = DarkBlue;
	palette[i++] = MediumBlue;
	palette[i++] = Blue;
	palette[i++] = LightBlue;
	palette[i++] = DarkPurple;
	palette[i++] = MediumPurple;
	palette[i++] = Purple;
	palette[i] = LightPurple;

}

Whiteboard::~Whiteboard()
{
	ClearTempSurface();

	if (pBackBuffer)
	{
		pBackBuffer->Release();
		pBackBuffer = nullptr;
	}	
	if (pDevice)
	{
		pDevice->Release();
		pDevice = nullptr;
	}
	if (pDirect3D)
	{
		pDirect3D->Release();
		pDirect3D = nullptr;
	}
}
