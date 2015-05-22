#include "Whiteboard.h"
#include "CNLIB\Messages.h"
#include <assert.h>
#include "CNLIB\HeapAlloc.h"

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

	BeginFrame();
	pDevice->Clear(0, NULL, D3DCLEAR_TARGET, bgColor, 0.0f, 0);
	EndFrame();
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

void Whiteboard::Frame(const RectU &rect, const BYTE *pixelData)
{
	BeginFrame();
	Render(rect, pixelData);
	EndFrame();
}

void Whiteboard::SendMouseData(MouseServer& mServ, TCPClientInterface* client)
{
	if(timer.GetTimeMilli() >= interval)
	{
		MouseClient mClient(mServ);
		if(!mClient.MouseEmpty())
		{
			const DWORD nBytes = mServ.GetBufferLen() + MSG_OFFSET;
			char* msg = alloc<char>(nBytes);
			msg[0] = TYPE_DATA;
			msg[1] = MSG_DATA_MOUSE;
			mServ.Extract((BYTE*)&msg[MSG_OFFSET]);
			HANDLE hnd = client->SendServData(msg, nBytes);
			WaitAndCloseHandle(hnd);
			dealloc(msg);
		}
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

void Whiteboard::Render(const RECT &rect, const BYTE *pixelData)
{
	const USHORT width = rect.right - rect.left;
	const USHORT height = rect.bottom - rect.top;

	ComposeImage(width, height, pixelData);

	for (USHORT y = 0; y < height; y++)
	{
		// Pitch won't always be the same as width * 4
		const UINT bRowOffset = (y * pitch) + rect.top;
		const UINT rectRowOffset = y * width;

		for (USHORT x = 0; x < pitch; x++)
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

void Whiteboard::ComposeImage(USHORT Width, USHORT Height, const BYTE *pixelData)
{
	HRESULT hr = 0;
	tempSurface =  alloc<D3DCOLOR>(Width * Height);

	for (USHORT y = 0; y < height; y++)
	{
		const UINT rowOffset = y * Width;
		for (USHORT x = 0; x < Width; x++)
		{
			const UINT index = x * rowOffset;
			tempSurface[index] = palette.GetRGBColor(pixelData[index]);
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
