#include "StdAfx.h"
#include "Whiteboard.h"
//#include "CNLIB\HeapAlloc.h"
#include "CNLIB\MsgStream.h"
#include "MessagesExt.h"

#pragma comment(lib, "d3d9.lib")

typedef Whiteboard::D3D D3D;

D3D::D3D(USHORT width, USHORT height)
	:
	hWnd(NULL),
	width(width),
	height(height),
	pitch(0),
	pDirect3D(nullptr),
	pDevice(nullptr),
	pBackBuffer(nullptr),
	lockRect()
{}
D3D::D3D(D3D&& d3d)
	:
	hWnd(d3d.hWnd),
	width(d3d.width),
	height(d3d.height),
	pitch(d3d.pitch),
	pDirect3D(d3d.pDirect3D),
	pDevice(d3d.pDevice),
	pBackBuffer(d3d.pBackBuffer),
	lockRect(d3d.lockRect)
{
	ZeroMemory(&d3d, sizeof(D3D));
}

D3D::~D3D()
{
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

void D3D::Initialize(HWND WinHandle)
{
	hWnd = WinHandle;
	HRESULT hr = S_OK;

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
			D3DCREATE_MIXED_VERTEXPROCESSING | D3DCREATE_PUREDEVICE, &d3dpp, &pDevice);
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

	pitch = (USHORT)lockRect.Pitch / width;

	hr = pBackBuffer->UnlockRect();
	assert(SUCCEEDED(hr));
}
void D3D::Draw(const RectU &rect, const BYTE *pixelData, const Palette& palette)
{
	const size_t rHeight = rect.bottom - rect.top;
	const size_t rWidth = rect.right - rect.left;
	for (size_t y = 0; y < rHeight; y++)
	{
		for (size_t x = 0; x < rWidth; x++)
		{
			D3DCOLOR* d3dSurf = (D3DCOLOR*)((char*)lockRect.pBits + pitch * ((x + rect.left) + (y + rect.top) * width));
			*d3dSurf = palette.GetRGBColor(pixelData[x + y * rWidth]);
		}
	}
}

void D3D::BeginFrame()
{
	HRESULT hr = pBackBuffer->LockRect(&lockRect, nullptr, NULL);
	assert(SUCCEEDED(hr));
}
void D3D::EndFrame()
{
	HRESULT hr = 0;
	hr = pBackBuffer->UnlockRect();
	assert(SUCCEEDED(hr));

	hr = pDevice->Present(nullptr, nullptr, nullptr, nullptr);
	assert(SUCCEEDED(hr));
}

void D3D::Clear(D3DCOLOR clr)
{
	pDevice->Clear(0, NULL, D3DCLEAR_TARGET, clr, 0.0f, 0);
}

USHORT D3D::GetWidth() const
{
	return width;
}
USHORT D3D::GetHeight() const
{
	return height;
}

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
	return 0;
}


Whiteboard::Whiteboard(TCPClientInterface& clint, Palette& palette, USHORT Width, USHORT Height, USHORT FPS, BYTE palIndex)
	:
	d3d(Width, Height),
	clint(clint),
	mouse((FPS < 60 ? 4500 / FPS : 75), (USHORT)((1000.0f / (float)FPS) + 0.5f)),
	defaultColor(palIndex),
	palIndex(palIndex),
	brushThickness(1.0f),
	interval(1.0f / (float)FPS),
	timer(CreateWaitableTimer(NULL, FALSE, NULL)),
	thread(NULL),
	threadID(NULL),
	palette(palette)
{}

Whiteboard::Whiteboard(Whiteboard&& wb)
	:
	d3d(wb.d3d),
	clint(wb.clint),
	mouse(std::move(wb.mouse)),
	defaultColor(wb.defaultColor),
	palIndex(wb.palIndex),
	brushThickness(wb.brushThickness),
	interval(wb.interval),
	timer(wb.timer),
	thread(wb.thread),
	threadID(wb.threadID),
	palette(wb.palette)
{
	thread = NULL;
	ZeroMemory(&wb, sizeof(Whiteboard));
}

void Whiteboard::Initialize(HWND WinHandle)
{
	d3d.Initialize(WinHandle);

	d3d.BeginFrame();
	d3d.Clear(palette.GetRGBColor(palIndex));
	d3d.EndFrame();
}

void Whiteboard::Frame(const RectU &rect, const BYTE *pixelData)
{
	d3d.BeginFrame();
	d3d.Draw(rect, pixelData, palette);
	d3d.EndFrame();
}

MouseServer& Whiteboard::GetMServ()
{
	return mouse;
}

void Whiteboard::SendMouseData(TCPClientInterface* client)
{
	MouseClient mClient(mouse);
	if(!mClient.MouseEmpty())
	{
		UINT count;
		const DWORD nBytes = mouse.GetBufferLen(count) + MSG_OFFSET;
		char* msg = alloc<char>(nBytes);
		MsgStreamWriter streamWriter = client->CreateOutStream(nBytes, TYPE_DATA, MSG_DATA_MOUSE);
		//*((short*)msg) = TYPE_DATA;
		//*((short*)msg + 1) = MSG_DATA_MOUSE;
		mouse.Extract((BYTE*)(streamWriter + MSG_OFFSET), count);
		client->SendServData(streamWriter);
		dealloc(msg);
	}
}

void Whiteboard::ChangeTool(Tool tool, float brushSize, BYTE palIndex)
{
	brushThickness = brushSize;

	const DWORD nBytes = sizeof(Tool) + sizeof(float) + sizeof(BYTE);
	auto streamWriter = clint.CreateOutStream(nBytes, TYPE_TOOL, MSG_TOOL_CHANGE);
	streamWriter.Write(tool);
	streamWriter.Write(brushSize);
	streamWriter.Write(palIndex);
	clint.SendServData(streamWriter);
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

D3D& Whiteboard::GetD3D()
{
	return d3d;
}

Whiteboard::~Whiteboard()
{
	if(thread)
	{
		PostThreadMessage(threadID, WM_QUIT, 0, 0);
		WaitAndCloseHandle(thread);
	}
}
