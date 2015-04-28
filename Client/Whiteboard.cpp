#include "Whiteboard.h"
#include <assert.h>
#include "..\\Common\\HeapAlloc.h"

Whiteboard::Whiteboard(Palette& palette, HWND WinHandle, USHORT Width, USHORT Height, USHORT FPS, BYTE palIndex)
	:
palette(palette),
hWnd(WinHandle),
width(Width),
height(Height),
interval(1.0f / (float)FPS),
tempSurface(nullptr)
{
	InitD3D();
	bgColor = palette.GetRGBColor(palIndex);
	pDevice->Clear(0, NULL, D3DCLEAR_TARGET, bgColor, 0.0f, 0);
}

Whiteboard::Whiteboard(Whiteboard&& wb)
	:
	hWnd(wb.hWnd),
	width(wb.width),
	height(wb.height),
	pitch(wb.pitch),
	interval(wb.interval),
	timer(wb.timer),
	bgColor(wb.bgColor),
	pDirect3D(wb.pDirect3D),
	pDevice(wb.pDevice),
	pBackBuffer(wb.pBackBuffer),
	tempSurface(wb.tempSurface),
	lockRect(wb.lockRect),
	palette(wb.palette)
{
	ZeroMemory(&wb, sizeof(Whiteboard));
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
	USHORT width = rect.right - rect.left;
	USHORT height = rect.bottom - rect.top;

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
}

void Whiteboard::EndFrame()
{
	HRESULT hr = 0;
	hr = pBackBuffer->UnlockRect();
	assert(SUCCEEDED(hr));

	hr = pDevice->Present(nullptr, nullptr, nullptr, nullptr);
	assert(SUCCEEDED(hr));
}

void Whiteboard::ComposeImage(USHORT Width, USHORT Height, BYTE *pixelData)
{
	HRESULT hr = 0;
	tempSurface =  alloc<D3DCOLOR>(Width * Height);
	
	for (int y = 0; y< height; y++)
	{
		UINT rowOffset = y * Width;
		for (int x = 0; x < Width; x++)
		{
			UINT index = x * rowOffset;
			UINT paletteIndex = pixelData[index];

			tempSurface[index] = palette.GetRGBColor(paletteIndex);
		}
	}
}

void Whiteboard::ClearTempSurface()
{
	dealloc(tempSurface);
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
	
	BOOL isWindow = IsWindow(hWnd);
	d3dpp.hDeviceWindow = hWnd;

	hr = pDirect3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE, &d3dpp, &pDevice);
	if (FAILED(hr))
	{
		hr = pDirect3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
			D3DCREATE_MIXED_VERTEXPROCESSING| D3DCREATE_PUREDEVICE, &d3dpp, &pDevice);
	}
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

const Palette const &Whiteboard::GetPalette(BYTE& count)
{
	count = 32;
	return palette;
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
