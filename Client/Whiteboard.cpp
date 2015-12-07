#include "Whiteboard.h"
#include "CNLIB\Messages.h"
#include <assert.h>
#include "CNLIB\HeapAlloc.h"
#include "CNLIB\MsgStream.h"

struct WBThreadParams
{
	WBThreadParams(Whiteboard* wb, TCPClientInterface* client)
		:
		wb(wb),
		client(client)
	{}

	WBThreadParams(WBThreadParams&& params)
		:
		wb(params.wb),
		client(params.client)
	{
		ZeroMemory(&params, sizeof(WBThreadParams));
	}

	~WBThreadParams(){}

	Whiteboard* wb;
	TCPClientInterface* client;
};

DWORD CALLBACK WBThread(LPVOID param)
{
	WBThreadParams* wbParams = (WBThreadParams*)param;
	Whiteboard& wb = *(Whiteboard*)(wbParams->wb);
	HANDLE timer = wb.GetTimer();

	while(true)
	{
		const DWORD ret = MsgWaitForMultipleObjects(1, &timer, FALSE, INFINITE, QS_ALLINPUT);
		if(ret == WAIT_OBJECT_0)
		{
			wb.SendMouseData(wbParams->client);
		}

		else if(ret == WAIT_OBJECT_0 + 1)
		{
			MSG msg;
			while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) != 0)
			{
				if(msg.message == WM_QUIT)
				{
					CancelWaitableTimer(timer);
					CloseHandle(timer);

					destruct(wbParams);
					return 0;
				}
			}
		}

		else
		{
			destruct(wbParams);
			return 0;
		}
	}

}


Whiteboard::Whiteboard(TCPClientInterface& clint, Palette& palette, USHORT Width, USHORT Height, USHORT FPS, BYTE palIndex)
	:
clint(clint),
mouse((FPS < 60 ? 4500 / FPS : 75), (USHORT)((1000.0f / (float)FPS) + 0.5f)),
defaultColor(palIndex),
palIndex(palIndex),
hWnd(NULL),
width(Width),
height(Height),
pitch(0),
brushThickness(1.0f),
interval(1.0f / (float)FPS),
timer(CreateWaitableTimer(NULL, FALSE, NULL)),
thread(NULL),
threadID(NULL),
palette(palette),
pDirect3D(nullptr),
pDevice(nullptr),
pBackBuffer(nullptr),
lockRect()
{}

Whiteboard::Whiteboard(Whiteboard&& wb)
	:
	clint(wb.clint),
	mouse(std::move(wb.mouse)),
	defaultColor(wb.defaultColor),
	palIndex(wb.palIndex),
	hWnd(wb.hWnd),
	width(wb.width),
	height(wb.height),
	pitch(wb.pitch),
	brushThickness(wb.brushThickness),
	interval(wb.interval),
	timer(wb.timer),
	thread(wb.thread),
	threadID(wb.threadID),
	pDirect3D(wb.pDirect3D),
	pDevice(wb.pDevice),
	pBackBuffer(wb.pBackBuffer),
	lockRect(wb.lockRect),
	palette(wb.palette)
{
	thread = NULL;
}

void Whiteboard::Initialize(HWND WinHandle)
{
	hWnd = WinHandle;
	InitD3D();

	BeginFrame();
	pDevice->Clear(0, NULL, D3DCLEAR_TARGET, palette.GetRGBColor(palIndex), 0.0f, 0);
	EndFrame();
}

void Whiteboard::Frame(const RectU &rect, const BYTE *pixelData)
{
	BeginFrame();
	Draw(rect, pixelData);
	EndFrame();
}

MouseServer& Whiteboard::GetMServ()
{
	return mouse;
}

USHORT Whiteboard::GetWidth() const
{
	return width;
}

USHORT Whiteboard::GetHeight() const
{
	return height;
}


void Whiteboard::SendMouseData(TCPClientInterface* client)
{
	MouseClient mClient(mouse);
	if(!mClient.MouseEmpty())
	{
		UINT count;
		const DWORD nBytes = mouse.GetBufferLen(count) + MSG_OFFSET;
		char* msg = alloc<char>(nBytes);
		msg[0] = TYPE_DATA;
		msg[1] = MSG_DATA_MOUSE;
		mouse.Extract((BYTE*)(msg + MSG_OFFSET), count);
		client->SendServData(msg, nBytes);
		dealloc(msg);
	}
}

void Whiteboard::BeginFrame()
{
	HRESULT hr = 0;
	hr = pBackBuffer->LockRect(&lockRect, nullptr, NULL);
	assert(SUCCEEDED(hr));
}

void Whiteboard::Draw(const RECT &rect, const BYTE *pixelData)
{
	const size_t rHeight = rect.bottom - rect.top;
	const size_t rWidth = rect.right - rect.left;
	for(size_t y = 0; y < rHeight; y++)
	{
		for(size_t x = 0; x < rWidth; x++)
		{
			D3DCOLOR* d3dSurf = (D3DCOLOR*)&((char*)lockRect.pBits)[pitch * ((x + rect.left) + (y + rect.top) * width)];
			*d3dSurf = palette.GetRGBColor(pixelData[x + y * rWidth]);
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


void Whiteboard::ChangeTool(Tool tool, float brushSize, BYTE palIndex)
{
	brushThickness = brushSize;

	const DWORD nBytes = sizeof(Tool) + sizeof(float) + sizeof(BYTE);
	MsgStreamWriter streamWriter(TYPE_TOOL, MSG_TOOL_CHANGE, nBytes);
	streamWriter.Write(tool);
	streamWriter.Write(brushSize);
	streamWriter.Write(palIndex);
	clint.SendServData(streamWriter, streamWriter.GetSize());
}

const Palette &Whiteboard::GetPalette(BYTE& count)
{
	count = 32;
	return palette;
}

float Whiteboard::GetBrushThickness() const
{
	return brushThickness;
}


HANDLE Whiteboard::GetTimer() const
{
	return timer;
}

BYTE Whiteboard::GetDefaultColor() const
{
	return defaultColor;
}


void Whiteboard::StartThread(TCPClientInterface* client)
{
	LARGE_INTEGER LI;
	LI.QuadPart = (LONGLONG)(interval * -10000000.0f);
	SetWaitableTimer(timer, &LI, (LONG)(interval * 1000.0f), NULL, NULL, FALSE);

	thread = CreateThread(NULL, 0, WBThread, construct<WBThreadParams>(this, client), NULL, &threadID);
}


Whiteboard::~Whiteboard()
{
	if(thread)
	{
		PostThreadMessage(threadID, WM_QUIT, 0, 0);
		WaitAndCloseHandle(thread);

		if(pBackBuffer)
		{
			pBackBuffer->Release();
			pBackBuffer = nullptr;
		}
		if(pDevice)
		{
			pDevice->Release();
			pDevice = nullptr;
		}
		if(pDirect3D)
		{
			pDirect3D->Release();
			pDirect3D = nullptr;
		}
	}
}
