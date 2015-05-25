#include "Whiteboard.h"
#include "CNLIB\Messages.h"
#include <assert.h>
#include "CNLIB\HeapAlloc.h"

Whiteboard::Whiteboard(Palette& palette, USHORT Width, USHORT Height, USHORT FPS, BYTE palIndex)
	:
surf(alloc<BYTE>(Width * Height)),
palIndex(palIndex),
hWnd(NULL),
width(Width),
height(Height),
interval(1000.0f / (float)FPS),
palette(palette)
{

}

Whiteboard::Whiteboard(Whiteboard&& wb)
	:
	surf(wb.surf),
	palIndex(wb.palIndex),
	hWnd(wb.hWnd),
	width(wb.width),
	height(wb.height),
	pitch(wb.pitch),
	interval(wb.interval),
	timer(wb.timer),
	pDirect3D(wb.pDirect3D),
	pDevice(wb.pDevice),
	pBackBuffer(wb.pBackBuffer),
	lockRect(wb.lockRect),
	palette(wb.palette)
{
	ZeroMemory(&wb, sizeof(Whiteboard));
}

void Whiteboard::Initialize(HWND WinHandle)
{
	hWnd = WinHandle;
	InitD3D();
	memset(surf, palIndex, width * height);

	BeginFrame();
	pDevice->Clear(0, NULL, D3DCLEAR_TARGET, palette.GetRGBColor(palIndex), 0.0f, 0);
	EndFrame();
}

void Whiteboard::Frame(const RectU &rect, const BYTE *pixelData)
{
	Draw(rect, pixelData);
}

bool Whiteboard::Interval() const
{
	return timer.GetTimeMilli() >= interval;
}

USHORT Whiteboard::GetWidth() const
{
	return width;
}

USHORT Whiteboard::GetHeight() const
{
	return height;
}


void Whiteboard::SendMouseData(MouseServer& mServ, TCPClientInterface* client)
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

		timer.Reset();
	}
}

void Whiteboard::BeginFrame()
{
	HRESULT hr = 0;
	hr = pBackBuffer->LockRect(&lockRect, nullptr, NULL);
	assert(SUCCEEDED(hr));
}

bool Whiteboard::Initialized() const
{
	return hWnd;
}

void Whiteboard::Render()
{
	for(USHORT y = 0; y < height; y++)
	{
		for(USHORT x = 0; x < width; x++)
		{
			const UINT index = x + (y * width);
			D3DCOLOR* d3dSurf = (D3DCOLOR*)&((char*)lockRect.pBits)[index * pitch];
			*d3dSurf = palette.GetRGBColor(surf[index]);
		}
	}
}

void Whiteboard::Draw(const RECT &rect, const BYTE *pixelData)
{
	const USHORT rWidth = rect.right - rect.left;
	const USHORT rHeight = rect.bottom - rect.top;

	for(USHORT y = 0; y < rHeight; y++)
	{
		for(USHORT x = 0; x < rWidth; x++)
		{
			const UINT bIndex = (x + rect.left) + ((y + rect.top) * width);
			surf[bIndex] = pixelData[x + (y * rWidth)];
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

//void Whiteboard::ComposeImage(USHORT Width, USHORT Height, const BYTE *pixelData)
//{
//	tempSurface =  alloc<D3DCOLOR>(Width * Height);
//
//	for (USHORT y = 0; y < Height; y++)
//	{
//		for (USHORT x = 0; x < Width; x++)
//		{
//			const UINT index = x + (y * Width);
//			tempSurface[index] = palette.GetRGBColor(pixelData[index]);
//		}
//	}
//}

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

	hr = pBackBuffer->LockRect(&lockRect, nullptr, NULL);
	assert(SUCCEEDED(hr));

	pitch = lockRect.Pitch / width;

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
	dealloc(surf);

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
