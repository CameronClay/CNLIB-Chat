// Client version of Windows.cpp
#include "resource.h"
#include "CNLIB\Typedefs.h"
#include <ws2tcpip.h>
#include "CNLIB\MsgStream.h"
#include "CNLIB\File.h"
#include "FileTransfer.h"
#include "Format.h"
#include "CNLIB\HeapAlloc.h"
#include "CNLIB\Messages.h"
#include "Options.h"
#include "Whiteboard.h"
#include "Mouse.h"
#include "DebugHelper.h"
#include "WhiteboardClientData.h"
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdlib.h>
#include <string.h>
#include <list>
#include <assert.h>
#include <math.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "TCPCS.lib")


const float APPVERSION = .002f;
const float CONFIGVERSION = .002f;
const USHORT DEFAULTPORT = 565;

#pragma region WindowClassIDs
#define ID_SERV_CONNECT 0
#define ID_SERV_MANAGE 1
#define ID_SERV_DISCONNECT 2

#define ID_VIEW_OPTIONS 3
#define ID_VIEW_LOGS 4

#define ID_TEXTBOX_DISPLAY 5
#define ID_TEXTBOX_INPUT 6

#define ID_LISTBOX_CLIENTS 7

#define ID_BUTTON_ENTER 8

#define ID_SEND_FILE 9
#define ID_SEND_FOLDER 10
#define ID_KICK 11

#define ID_TIMER_INACTIVE 0

#define ID_WHITEBOARD_START 99
#define ID_WHITEBOARD_TERMINATE 100
#define ID_WHITEBOARD_KICK 101
#define ID_WHITEBOARD_SETTINGS 102
#define ID_WHITEBOARD_INVITE 103

#pragma endregion

#define WB_DEF_RES_X 800
#define WB_DEF_RES_Y 600
#define WB_DEF_FPS 60

#define WM_CREATEWIN WM_APP
#define ID_WB 1

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WbProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK ConnectProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK ManageProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK AuthenticateProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK Opt_GeneralProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK Opt_TextProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK Opt_FilesProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK RequestFileProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK RequestWBProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK TimerProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK WBSettingsProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK WBInviteProc(HWND, UINT, WPARAM, LPARAM);

#pragma region GlobalVarDeclarations
HINSTANCE hInst;
HWND hMainWind;

USHORT screenWidth = 800, screenHeight = 600;
USHORT left = 0, leftLen = 0;
USHORT top = 0, topLen = 0;

TCPClientInterface* client;
uqpc<Options> opts;
uqpc<FileSend> fileSend;
uqpc<FileReceive> fileReceive;

static LIB_TCHAR szWindowClass[] = _T("Client");
static LIB_TCHAR szTitle[] = _T("Cameron's Client");

static LIB_TCHAR windowName[56];

const LIB_TCHAR folderName[] = _T("Cam's-Client");
const LIB_TCHAR servListFileName[] = _T("ServrecvList.txt");
LIB_TCHAR servListFilePath[MAX_PATH + 50];

const LIB_TCHAR optionsFileName[] = _T("Options.dat");
LIB_TCHAR optionsFilePath[MAX_PATH + 50];

const UINT maxUserLen = 10;

const USHORT port = 565;
const float timeOut = 5.0f;

LIB_TCHAR folderPath[MAX_PATH + 30];

std::list<std::tstring> servList;

// Whiteboard declarations
Palette palette;
LIB_TCHAR wbClassName[] = _T("Whiteboard");
LIB_TCHAR wbWindowName[] = _T("Client Whiteboard View");
ATOM wbAtom = NULL;
static HMENU wbMenu = nullptr;
static HWND wbHandle = nullptr;
Whiteboard *pWhiteboard = nullptr;
// Whiteboard declarations end

// Mouse declaration
MouseServer mServ;
// Mouse declaration end

static HWND textDisp, textInput, listClients;
static HMENU main, file, options;

std::tstring user;
#pragma endregion

POINT pt = { 0, 0 };
double mouseX = 0.0, mouseY = 0.0;
double mouseSpeed = 1.0;
void UpdateMouse();


HWND CreateWBWindow(USHORT width, USHORT height)
{
	if(!wbAtom)
	{
		WNDCLASS wc = {};
		wc.hInstance = hInst;
		wc.lpfnWndProc = WbProc;
		wc.lpszClassName = wbClassName;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);

		wbAtom = RegisterClass(&wc);
	}

	// Test whether the whiteboard handle is valid so that it can 
	// safely be recreated if it gets closed

	if(!IsWindow(wbHandle))
	{
		/*RECT screen;
		SystemParametersInfo(SPI_GETWORKAREA, NULL, &screen, NULL);*/

		RECT wr;
		wr.left = 0;
		wr.right = wr.left + width;
		wr.top = 0;
		wr.bottom = wr.top + height;
		AdjustWindowRect(&wr, WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU, FALSE);
		DWORD style =
			//WS_CHILD |			// Child window that
			WS_POPUPWINDOW |	// Is not contained in parent window
			WS_CAPTION |		// Shows the title bar
			WS_MINIMIZEBOX;		// Shows the minimize button
		
		wbHandle = CreateWindowEx(
			NULL,
			wbClassName,
			_T("Whiteboard Client View"),
			style,
			0, 0,
			wr.right - wr.left, wr.bottom - wr.top,
			NULL, NULL,
			hInst, nullptr);

		if(!wbHandle)
		{
			CheckForError(_T("Whiteboard Window Creation"));
		}
	}

	ShowWindow(wbHandle, SW_SHOWNORMAL);
	return wbHandle;
}

void RecalcSizeVars(USHORT width, USHORT height)
{
	screenWidth = width;
	screenHeight = height;
	left = screenWidth * (8.75f / 10.0f), leftLen = (screenWidth - left) - 10;
	top = screenHeight * (8.0f / 10.0f), topLen = screenHeight - top;
}

void DispText(BYTE* data, DWORD nBytes)
{
	const UINT len = SendMessage(textDisp, WM_GETTEXTLENGTH, 0, 0) + 1;
	LIB_TCHAR* buffer = (LIB_TCHAR*)alloc<char>(((len + 2) * sizeof(LIB_TCHAR)) + nBytes);
	SendMessage(textDisp, WM_GETTEXT, len, (LPARAM)buffer);
	if (len != 1) _tcscat(buffer, _T("\r\n"));
	_tcscat(buffer, (LIB_TCHAR*)data);
	SendMessage(textDisp, WM_SETTEXT, 0, (LPARAM)buffer);
	dealloc(buffer);
	SendMessage(textDisp, EM_LINESCROLL, 0, MAXLONG);
}

void Connect(const LIB_TCHAR* dest, const LIB_TCHAR* port, float timeOut)
{
	client->Connect(dest, port, timeOut);
	if (client->IsConnected())
	{
		EnableMenuItem(file, ID_SERV_CONNECT, MF_GRAYED);
		EnableMenuItem(file, ID_SERV_DISCONNECT, MF_ENABLED);
	}
}

void ClearAll()
{
	//SendMessage(textDisp, WM_SETTEXT, 0, (LPARAM)_T(""));
	SendMessage(textInput, WM_SETTEXT, 0, (LPARAM)_T(""));
	SendMessage(listClients, LB_RESETCONTENT, 0, (LPARAM)_T(""));
}

void Disconnect()
{
	client->Disconnect();
}

void Flash()
{
	// I put FLASHWINFO in Options
	opts->FlashTaskbar(hMainWind);
}

//returns index if found, -1 if not found
int FindClient(std::tstring& name)
{
	const USHORT count = SendMessage(listClients, LB_GETCOUNT, 0, 0);
	LIB_TCHAR* buffer = alloc<LIB_TCHAR>(maxUserLen + 1);
	int found = -1;

	for(USHORT i = 0; i < count; i++)
	{
		SendMessage(listClients, LB_GETTEXT, i, (LPARAM)buffer);
		if(name.compare(buffer) == 0)
		{
			found = i;
			break;
		}
	}

	dealloc(buffer);
	return found;
}

void MsgHandler(void* clientObj, BYTE* data, DWORD nBytes, void* obj)
{
	//HRESULT res = CoInitialize(NULL);
	//assert(SUCCEEDED(res));

	TCPClientInterface& clint = *(TCPClientInterface*)clientObj;
	char* dat = (char*)(&data[MSG_OFFSET]);
	nBytes -= MSG_OFFSET;
	MsgStreamReader streamReader((char*)data, nBytes);
	const char type = streamReader.GetType(), msg = streamReader.GetMsg();

	switch (type)
	{
		case TYPE_PING:
		{
			switch(msg)
			{
			case MSG_PING:
				clint.Ping();
				break;
			}
			break;
		}//TYPE_PING
		case TYPE_CHANGE:
		{
			switch(msg)
			{
			case MSG_CHANGE_SERVERFULL:
			{
				Flash();
				MessageBox(hMainWind, _T("Server is full!"), _T("ERROR"), MB_ICONERROR);
				Disconnect();
				break;
			}
			case MSG_CHANGE_CONNECT:
			{
				LIB_TCHAR buffer[128];
				_stprintf(buffer, _T("Server: %s has connected!"), (LIB_TCHAR*)dat);

				UINT nBy = 0;
				LIB_TCHAR* text = FormatText((BYTE*)buffer, _tcslen(buffer) * sizeof(LIB_TCHAR), nBy, opts->TimeStamps());
				DispText((BYTE*)text, nBy);
				dealloc(text);
				std::tstring str = (LIB_TCHAR*)dat;
				const UINT first = str.find(_T("<"), 0) + 1, second = str.find(_T(">"), first), len = second - first;
				assert((first != std::tstring::npos) || (second != std::tstring::npos));
				const std::tstring name = str.substr(first, len);

				if(FindClient((std::tstring)name) == -1)
					SendMessage(listClients, LB_ADDSTRING, 0, (LPARAM)name.c_str());

				Flash();
				break;
			}
			case MSG_CHANGE_CONNECTINIT:
			{
				if(FindClient(std::tstring((LIB_TCHAR*)dat)) == -1)
					SendMessage(listClients, LB_ADDSTRING, 0, (LPARAM)dat);

				break;
			}
			case MSG_CHANGE_DISCONNECT:
			{
				std::tstring name = (LIB_TCHAR*)dat;
				int item = 0;

				if(fileReceive->Running())
				{
					if(name.compare(fileReceive->GetUser()) == 0)
					{
						fileReceive->StopReceive();
					}
				}
				if(fileSend->Running())
				{
					if(name.compare(fileSend->GetUser()) == 0)
					{
						fileSend->StopSend();	
						//fileSend->RunCanceled(); not required because of msgloop in sendfiles
					}
				}

				if((item = FindClient((std::tstring)name)) != -1)
					SendMessage(listClients, LB_DELETESTRING, item, 0);

				LIB_TCHAR buffer[128];
				_stprintf(buffer, _T("Server: %s has disconnected!"), (LIB_TCHAR*)dat);


				UINT nBy = 0;
				LIB_TCHAR* text = FormatText((BYTE*)buffer, _tcslen(buffer) * sizeof(LIB_TCHAR), nBy, opts->TimeStamps());
				DispText((BYTE*)text, nBy);
				dealloc(text);
				Flash();
				break;
			}
			}
			break;
		}//TYPE_CHANGE
		case TYPE_DATA:
		{
			switch (msg)
			{
				case MSG_DATA_TEXT:
				{
					UINT nBy = 0;
					LIB_TCHAR* buffer = FormatText((BYTE*)dat, nBytes, nBy, opts->TimeStamps());
					DispText((BYTE*)buffer, nBy);
					dealloc(buffer);
					Flash();
					break;
				}
				case MSG_DATA_BITMAP:
				{
					RectU& rect = streamReader.Read<RectU>();
					BYTE* pixels = streamReader.ReadEnd<BYTE>();
					pWhiteboard->Frame(rect, pixels);
					break;
				}
			}
			break;
		}//TYPE_DATA
		case TYPE_RESPONSE:
		{
			switch (msg)
			{
				case MSG_RESPONSE_AUTH_DECLINED:
				{
					EnableWindow(hMainWind, TRUE);
					MessageBox(hMainWind, _T("Invalid username/password or username is already taken"), _T("ERROR"), MB_ICONERROR);
					Disconnect();
					break;
				}

				case MSG_RESPONSE_AUTH_CONFIRMED:
				{
					SendMessage(listClients, LB_ADDSTRING, 0, (LPARAM)user.c_str());
					EnableWindow(hMainWind, TRUE);
					MessageBox(hMainWind, _T("Success"), _T("Success"), MB_OK);
					break;
				}

				case MSG_RESPONSE_TRANSFER_DECLINED:
				{
					fileSend->Stop();//stop instead of stopsend because thread hasnt been created and only vars need cleared

					LIB_TCHAR buffer[255];
					_stprintf(buffer, _T("%s has declined your transfer request!"), (LIB_TCHAR*)dat);
					MessageBox(hMainWind, buffer, _T("DECLINED"), MB_ICONERROR);
					break;
				}

				case MSG_RESPONSE_TRANSFER_CONFIRMED:
				{
					LIB_TCHAR buffer[255];
					_stprintf(buffer, _T("%s has confirmed your transfer request!"), (LIB_TCHAR*)dat);
					MessageBox(hMainWind, buffer, _T("Success"), MB_OK);
					fileSend->StartSend();
					break;
				}

				case MSG_RESPONSE_WHITEBOARD_CONFIRMED:
				{
					LIB_TCHAR buffer[255];
					_stprintf(buffer, _T("%s has joined the Whiteboard!"), (LIB_TCHAR*)dat);
					MessageBox(hMainWind, buffer, _T("Success"), MB_OK);
					break;
				}

				case MSG_RESPONSE_WHITEBOARD_DECLINED:
				{
					LIB_TCHAR buffer[255];
					_stprintf(buffer, _T("%s has declined the whiteboard!"), (LIB_TCHAR*)dat);
					MessageBox(hMainWind, buffer, _T("DECLINED"), MB_ICONERROR);
					break;
				}

			}// MSG_RESPONES
			break;
		}//TYPE_RESPONSE

		case TYPE_FILE:
		{
			switch (msg)
			{
				case MSG_FILE_LIST:
				{
					fileReceive->RecvFileNameList(streamReader, opts->GetDownloadPath());
					break;
				}
				case MSG_FILE_DATA:
				{
					fileReceive->RecvFile((BYTE*)dat, nBytes);
					break;
				}
				case MSG_FILE_SEND_CANCELED:
				{
					fileReceive->StopReceive();
					fileReceive->RunCanceled();
					break;
				}
				case MSG_FILE_RECEIVE_CANCELED:
				{
					fileSend->StopSend();
					//fileSend->RunCanceled(); not required because of msgloop in sendfiles
				}
			}// MSG_FILE

			break;
		}//TYPE_FILE
		case TYPE_REQUEST:
		{
			switch (msg)
			{
				case MSG_REQUEST_TRANSFER:
				{
					const UINT len = streamReader.Read<UINT>();
					fileReceive->GetUser() = streamReader.Read<LIB_TCHAR>(len);
					fileReceive->SetSize(streamReader.Read<double>());
					if (fileReceive->Running())
					{
						client->SendMsg(fileReceive->GetUser(), TYPE_RESPONSE, MSG_RESPONSE_TRANSFER_DECLINED);
						break;
					}

					Flash();
					LIB_TCHAR buffer[255];
					_stprintf(buffer, _T("%s wants to send you %g MB of file(s)"), fileReceive->GetUser().c_str(), fileReceive->GetSize());

					DialogBoxParam(hInst, MAKEINTRESOURCE(REQUEST), hMainWind, RequestFileProc, (LPARAM)buffer);
					break;
				}
				case MSG_REQUEST_WHITEBOARD:
				{
					LIB_TCHAR buffer[255];
					_stprintf(buffer, _T("%s wants to display a whiteboard"), (LIB_TCHAR*)dat, _tcslen((LIB_TCHAR*)dat));

					DialogBoxParam(hInst, MAKEINTRESOURCE(REQUEST), hMainWind, RequestWBProc, (LPARAM)buffer);
					break;
				}
			}// MSG_REQUEST
			break;
		}//TYPE_REQUEST
		case TYPE_ADMIN:
		{
			switch (msg)
			{
				case MSG_ADMIN_NOT:
				{
					MessageBox(hMainWind, _T("You are not an admin"), _T("ERROR"), MB_ICONERROR);
					break;
				}
				case MSG_ADMIN_KICK:
				{
					if (fileReceive->Running())
					{
						fileReceive->StopReceive();
					}
					if (fileSend->Running())
					{
						fileSend->StopSend();
					}

					Flash();
					LIB_TCHAR buffer[255];
					_stprintf(buffer, _T("%s has kicked you from the server!"), (LIB_TCHAR*)dat);
					MessageBox(hMainWind, buffer, _T("Kicked"), MB_ICONERROR);

					Disconnect();
					break;
				}
				case MSG_ADMIN_CANNOTKICK:
				{
					MessageBox(hMainWind, _T("You cannot kick an admin!"), _T("ERROR"), MB_ICONERROR);
					break;
				}
			}// MSG_ADMIN

			break;
		}//TYPE_ADMIN
		case TYPE_VERSION:
		{
			switch (msg)
			{
				case MSG_VERSION_UPTODATE:
				{
					DialogBox(hInst, MAKEINTRESOURCE(AUTHENTICATE), hMainWind, AuthenticateProc);
					break;
				}
				case MSG_VERSION_OUTOFDATE:
				{
					Flash();
					MessageBox(hMainWind, _T("Your client's version does not match the server's version"), _T("UPDATE!"), MB_ICONERROR);
					Disconnect();
					break;
				}
			}// MSG_VERSION

			break;
		}//TYPE_VERSION

		case TYPE_WHITEBOARD:
		{
			switch(msg)
			{
			case MSG_WHITEBOARD_ACTIVATE:
			{
				/*UINT pos = 0;
				const USHORT width = *(USHORT*)(&dat[pos]);
				pos += sizeof(USHORT);

				const USHORT height = *(USHORT*)(&dat[pos]);
				pos += sizeof(USHORT);

				const USHORT FPS = *(USHORT*)(&dat[pos]);
				pos += sizeof(USHORT);

				const D3DCOLOR clr = *(D3DCOLOR*)(&dat[pos]);*/

				WBParams *pParams =	&streamReader.Read<WBParams>();

				pWhiteboard = construct(
					Whiteboard(
					palette,
					pParams->width,
					pParams->height,
					pParams->fps,
					pParams->clrIndex)
					);

				SendMessage(hMainWind, WM_CREATEWIN, ID_WB, (LPARAM)pParams);
				break;
			}
			case MSG_WHITEBOARD_TERMINATE:
			{
				// in case user declined
				if(IsWindow(wbHandle))
				{
					MessageBox(wbHandle, _T("Whiteboard has been shutdown!"), _T("Whiteboard Status"), MB_ICONINFORMATION);
					SendMessage(wbHandle, WM_CLOSE, 0, 0);
				}
				break;
			}
			case MSG_WHITEBOARD_CANNOTCREATE:
			{
				MessageBox(hMainWind, _T("A whiteboard is already being displayed!"), _T("ERROR"), MB_ICONHAND);
				break;
			}
			case MSG_WHITEBOARD_CANNOTTERMINATE:
			{
				MessageBox(hMainWind, _T("Only whiteboard creator can terminate the whiteboard!"), _T("ERROR"), MB_ICONSTOP);
				break;
			}
			case MSG_WHITEBOARD_KICK:
			{
				LIB_TCHAR buffer[255];
				_stprintf(buffer, _T("%s has removed you from the whiteboard!"), (LIB_TCHAR*)dat);
				MessageBox(wbHandle, buffer, _T("Kicked"), MB_ICONEXCLAMATION);
				SendMessage(wbHandle, WM_CLOSE, 0, 0);
				break;
			}
			}
			break;
		}

	}// TYPE
	/*CoUninitialize();*/
}

void DisconnectHandler() // for disconnection
{
	ClearAll();
	EnableMenuItem(file, ID_SERV_CONNECT, MF_ENABLED);
	EnableMenuItem(file, ID_SERV_DISCONNECT, MF_GRAYED);

	Flash();
	MessageBox(hMainWind, _T("You have been disconnected from server!"), _T("ERROR"), MB_ICONERROR);
}

void SaveServList()
{
	File file(servListFilePath, FILE_GENERIC_WRITE, FILE_ATTRIBUTE_HIDDEN, CREATE_ALWAYS);
	for (auto& i : servList)
	{
		file.WriteString(i);
	}
	file.Close();
}

void SendFinishedHandler(std::tstring& user)
{
	MessageBox(hMainWind, _T("File transfer finished"), _T("Success"), MB_OK);
}

void SendCanceledHandler(std::tstring& user)
{
	MessageBox(hMainWind, (std::tstring(_T("File transfer canceled by either you or ")) + user).c_str(), _T("ERROR"), MB_ICONERROR);
}

void ReceiveFinishedHandler(std::tstring& user)
{
	MessageBox(hMainWind, _T("File transfer finished"), _T("Success"), MB_OK);
}

void ReceiveCanceledHandler(std::tstring& user)
{
	MessageBox(hMainWind, (std::tstring(_T("File transfer canceled by either you or ")) + user).c_str(), _T("ERROR"), MB_ICONERROR);
}

void WinMainInit()
{
	FileMisc::GetFolderPath(CSIDL_APPDATA, folderPath);
	FileMisc::SetCurDirectory(folderPath);
	FileMisc::CreateFolder(folderName, FILE_ATTRIBUTE_HIDDEN);
	_stprintf(folderPath, _T("%s\\%s"), folderPath, folderName);
	FileMisc::SetCurDirectory(folderPath);

	_stprintf(servListFilePath, _T("%s\\%s"), folderPath, servListFileName);
	File file(servListFilePath, FILE_GENERIC_READ, FILE_ATTRIBUTE_HIDDEN);

	std::tstring temp;
	while(file.ReadString(temp))
	{
		servList.push_back(temp);
	}
	file.Close();


	_stprintf(optionsFilePath, _T("%s\\%s"), folderPath, optionsFileName);

	InitializeNetworking();

	client = CreateClient(&MsgHandler, &DisconnectHandler);
	fileSend = uqpc<FileSend>(construct<FileSend>(FileSend(*client, hMainWind, &SendFinishedHandler, &SendCanceledHandler)));
	fileReceive = uqpc<FileReceive>(construct<FileReceive>(FileReceive(*client, hMainWind, &ReceiveFinishedHandler, &ReceiveCanceledHandler)));
	opts = uqpc<Options>(construct<Options>(Options(std::tstring(optionsFilePath), CONFIGVERSION)));

	opts->Load(windowName);
}

int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{
	WinMainInit();

	HRESULT res = CoInitialize(NULL);
	assert(SUCCEEDED(res));

	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPLICATION));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_APPLICATION));

	_stprintf(windowName, _T("%s %.3f"), szTitle, APPVERSION);

	if (!RegisterClassEx(&wcex))
	{
		MessageBox(NULL, _T("Call to RegisterClassEx failed!"), windowName, NULL);
		return 1;
	}

	hInst = hInstance; 

	hMainWind = CreateWindow(
		szWindowClass,
		windowName,
		WS_OVERLAPPEDWINDOW,
		0, 0,
		screenWidth, screenHeight,
		NULL,
		NULL,
		hInstance,
		NULL
		);

	if (!hMainWind)
	{
		MessageBox(NULL, _T("Call to CreateWindow failed!"), windowName, NULL);
		return 1;
	}

	HACCEL hndAccel = LoadAccelerators(hInst, MAKEINTRESOURCE(HOTKEYS));

	ShowWindow(hMainWind, nCmdShow);
	UpdateWindow(hMainWind);

	/*RAWINPUTDEVICE rid;
	rid.usUsagePage = 0x01;
	rid.usUsage = 0x02;
	rid.dwFlags = NULL;
	rid.hwndTarget = NULL;

	RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE));*/

	MSG msg = {};
	while(msg.message != WM_QUIT)
	{
		if(pWhiteboard)
			UpdateMouse();

		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if(!TranslateAccelerator(hMainWind, hndAccel, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else if(pWhiteboard && pWhiteboard->Initialized() && pWhiteboard->Interval())
		{
			pWhiteboard->BeginFrame();
			pWhiteboard->Render();
			pWhiteboard->SendMouseData(mServ, client);
			pWhiteboard->EndFrame();
		}
	}

	return (int)msg.wParam;
}

void UpdateMouse()
{
	static POINT prevPt;

	BYTE buffer[512];
	UINT size = ARRAYSIZE(buffer);


	//GetRawInputBuffer(HRAWINPUT)lParam, RID_INPUT, ri, &size, sizeof(RAWINPUTHEADER));
	const UINT count = GetRawInputBuffer((RAWINPUT*)buffer, &size, sizeof(RAWINPUTHEADER));
	DWORD err = GetLastError();


	RAWINPUT *pri = (RAWINPUT*)buffer;
	for(UINT i = 0; i < count; i++)
	{
		if(pri->header.dwType == RIM_TYPEMOUSE)
		{
			RAWMOUSE& mouse = ((RAWINPUT*)((char*)pri + 8))->data.mouse;

			if(mouse.usFlags == MOUSE_MOVE_RELATIVE)
			{
				prevPt = pt;
				mouseX += mouseSpeed * (double)mouse.lLastX;
				mouseY += mouseSpeed * (double)mouse.lLastY;
				pt = { mouseX, mouseY };
			}

			else if(mouse.usFlags & MOUSE_MOVE_ABSOLUTE)
			{
				prevPt = pt;
				pt = { mouseX = mouse.lLastX, mouseY = mouse.lLastY };
			}

			RECT rc{};
			GetClientRect(wbHandle, &rc);
			if(PtInRect(&rc, pt))
			{
				POINT temp = pt;
				ScreenToClient(wbHandle, &temp);

				SetCursorPos(temp.x, temp.y);

				switch(mouse.usButtonFlags)
				{
				case RI_MOUSE_LEFT_BUTTON_DOWN:
					mServ.OnLeftPressed(temp.x, temp.y);
					break;

				case RI_MOUSE_LEFT_BUTTON_UP:
					mServ.OnLeftReleased(temp.x, temp.y);
					break;

				case RI_MOUSE_RIGHT_BUTTON_DOWN:
					mServ.OnRightPressed(temp.x, temp.y);
					break;

				case RI_MOUSE_RIGHT_BUTTON_UP:
					mServ.OnRightReleased(temp.x, temp.y);
					break;
				}

				if((pt.x != prevPt.x) || (pt.y != prevPt.y))
					mServ.OnMouseMove(temp.x, temp.y);
			}

			LIB_TCHAR textBuffer[128];
			_stprintf(textBuffer, _T("(%d, %d)"), pt.x, pt.y);
			SendMessage(textDisp, WM_SETTEXT, 0, (LPARAM)textBuffer);
		}
		pri = NEXTRAWINPUTBLOCK(pri);
	}
}

//DWORD CALLBACK Mouse(LPVOID)
//{
//	static POINT pt, prevPt;
//
//	RAWINPUTDEVICE rid;
//	rid.usUsagePage = 0x01;
//	rid.usUsage = 0x02;
//	rid.dwFlags = NULL;
//	rid.hwndTarget = wbHandle;
//
//	BOOL RESULT = RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE));
//
//	MSG msg = {};
//	while(msg.message != WM_QUIT)
//	{
//		if(!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
//		{
//			UINT size;
//			//GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER));
//			DWORD err = GetRawInputBuffer(NULL, &size, sizeof(RAWINPUTHEADER));
//
//			size = 16;
//			size += 8;
//			size *= 16;
//
//			RAWINPUT *ri = (RAWINPUT*)alloc<BYTE>(size);
//
//			//GetRawInputBuffer(HRAWINPUT)lParam, RID_INPUT, ri, &size, sizeof(RAWINPUTHEADER));
//			const UINT count = GetRawInputBuffer(ri, &size, sizeof(RAWINPUTHEADER));
//			err = GetLastError();
//
//
//			RAWINPUT *pri = ri;
//			for(UINT i = 0; i < count; i++)
//			{
//				if(pri->header.dwType == RIM_TYPEMOUSE)
//				{
//					RAWMOUSE& mouse = ((RAWINPUT*)((char*)pri + 8))->data.mouse;
//
//					if(mouse.usFlags == MOUSE_MOVE_RELATIVE)
//					{
//						prevPt = pt;
//						pt = { pt.x + mouse.lLastX, pt.y + mouse.lLastY };
//					}
//
//					else if(mouse.usFlags & MOUSE_MOVE_ABSOLUTE)
//					{
//						prevPt = pt;
//						pt = { mouse.lLastX, mouse.lLastY };
//					}
//
//					RECT rc{};
//					GetClientRect(wbHandle, &rc);
//					if(PtInRect(&rc, pt))
//					{
//						POINT temp = pt;
//						ScreenToClient(wbHandle, &temp);
//
//						switch(mouse.usButtonFlags)
//						{
//						case RI_MOUSE_LEFT_BUTTON_DOWN:
//							mServ.OnLeftPressed(temp.x, temp.y);
//							break;
//
//						case RI_MOUSE_LEFT_BUTTON_UP:
//							mServ.OnLeftReleased(temp.x, temp.y);
//							break;
//
//						case RI_MOUSE_RIGHT_BUTTON_DOWN:
//							mServ.OnRightPressed(temp.x, temp.y);
//							break;
//
//						case RI_MOUSE_RIGHT_BUTTON_UP:
//							mServ.OnRightReleased(temp.x, temp.y);
//							break;
//						}
//
//						if((pt.x != prevPt.x) || (pt.y != prevPt.y))
//							mServ.OnMouseMove(temp.x, temp.y);
//					}
//				}
//				pri = NEXTRAWINPUTBLOCK(pri);
//			}
//
//			LIB_TCHAR buffer[128];
//			_stprintf(buffer, _T("(%d, %d)"), pt.x, pt.y);
//			SendMessage(textDisp, WM_SETTEXT, 0, (LPARAM)buffer);
//			dealloc(ri);
//		}
//	}
//	return 0;
//}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND buttonEnter;
	const UINT nDialogs = 3;
	static PROPSHEETPAGE psp[nDialogs];

	switch (message)
	{
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case ID_TEXTBOX_INPUT:
			switch (HIWORD(wParam))
			{
			case EN_CHANGE:
				if (GetWindowTextLength(textInput) != 0)
					EnableWindow(buttonEnter, TRUE);
				else
					EnableWindow(buttonEnter, FALSE);
				break;
			}
			break;
		case ID_SERV_CONNECT:
			DialogBox(hInst, MAKEINTRESOURCE(CONNECT), hWnd, ConnectProc);
			break;

		case ID_SERV_DISCONNECT:
			Disconnect();
			break;

		case ID_SERV_MANAGE:
			DialogBox(hInst, MAKEINTRESOURCE(MANAGE), hWnd, ManageProc);
			break;

		case ID_VIEW_OPTIONS:
		{
			PROPSHEETHEADER psh{ sizeof(PROPSHEETHEADER) };
			psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
			psh.hwndParent = hWnd;
			psh.pszCaption = _T("Options");
			psh.nPages = nDialogs;
			psh.ppsp = psp;
			PropertySheet(&psh);
		}
		break;

		case ID_VIEW_LOGS:

			break;

		case ID_BUTTON_ENTER:
		case ENTER:
		{
			if (GetWindowTextLength(textInput) == 0) break;

			if (!client->IsConnected())
			{
				MessageBox(hMainWind, _T("Must be connected to server"), _T("ERROR"), MB_ICONERROR);
				break;
			}
			const UINT len = SendMessage(textInput, WM_GETTEXTLENGTH, 0, 0) + 1;
			const UINT nBytes = (len * sizeof(LIB_TCHAR)) + MSG_OFFSET;
			char* msg = alloc<char>(nBytes);
			msg[0] = TYPE_DATA;
			msg[1] = MSG_DATA_TEXT;
			SendMessage(textInput, WM_GETTEXT, len, (LPARAM)&msg[MSG_OFFSET]);
			SendMessage(textInput, WM_SETTEXT, 0, (LPARAM)_T(""));
			EnableWindow(buttonEnter, false);

			UINT nBy;
			LIB_TCHAR* dispMsg = FormatText((BYTE*)&msg[MSG_OFFSET], nBytes - MSG_OFFSET, user, nBy, opts->TimeStamps());
			DispText((BYTE*)dispMsg, nBy);
			dealloc(dispMsg);

			HANDLE hnd = client->SendServData(msg, nBytes);
			WaitAndCloseHandle(hnd);
			dealloc(msg);
			break;
		}

		case ID_SEND_FILE:
		{
			const UINT i = SendMessage(listClients, LB_GETCURSEL, 0, 0);
			const UINT len = SendMessage(listClients, LB_GETTEXTLEN, i, 0);
			fileSend->GetUser().resize(len);
			SendMessage(listClients, LB_GETTEXT, i, (LPARAM)&fileSend->GetUser()[0]);
			if (fileSend->GetUser().compare(user) == 0)
			{
				MessageBox(hMainWind, _T("One does not simply send files to ones self"), _T("ERROR"), MB_ICONERROR);
				break;
			}

			LIB_TCHAR buffer[MAX_PATH];
			LIB_TCHAR path[MAX_PATH];
			if (!FileMisc::BrowseFiles(_T("Browse files"), buffer, hWnd))
				break;

			_tcsncpy(path, buffer, _tcslen(buffer) + 1);
			PathRemoveFileSpec(path);
			fileSend->SetFullPathSrc(std::tstring(path) + _T("\\"));

			FileMisc::FileData data;
			File file(buffer, FILE_GENERIC_READ);
			data.dateModified = file.GetDate();
			data.size = file.GetSize();
			PathStripPath(buffer);
			data.fileName = buffer;
			file.Close();

			fileSend->GetList().push_back(data);

			fileSend->RequestTransfer();
			break;
		}

		case ID_SEND_FOLDER:
		{
			const UINT i = SendMessage(listClients, LB_GETCURSEL, 0, 0);
			const UINT len = SendMessage(listClients, LB_GETTEXTLEN, i, 0);
			fileSend->GetUser().resize(len);
			SendMessage(listClients, LB_GETTEXT, i, (LPARAM)&fileSend->GetUser()[0]);
			if (fileSend->GetUser().compare(user) == 0)
			{
				MessageBox(hMainWind, _T("One does not simply send files to ones self"), _T("ERROR"), MB_ICONERROR);
				break;
			}

			LIB_TCHAR buffer[MAX_PATH];
			if (!FileMisc::BrowseFolder(_T("Browse directories"), buffer, hWnd))
				break;

			auto& list = fileSend->GetList();

			FileMisc::GetFileNameList(buffer, NULL, list);
			if (!list.empty())
			{
				std::tstring path = buffer;
				const UINT pos = path.find_last_of(_T("\\"));
				fileSend->SetFullPathSrc(path.substr(0, pos) + _T("\\"));
				std::tstring lastPath = path.substr(pos + 1) + _T("\\");
				for (auto& i : list)
				{
					i.fileName.insert(0, lastPath);
				}

				fileSend->RequestTransfer();
			}
			break;
		}

		case ID_KICK:
		{
			const UINT i = SendMessage(listClients, LB_GETCURSEL, 0, 0);
			const UINT len = SendMessage(listClients, LB_GETTEXTLEN, i, 0);
			std::tstring userKick;

			userKick.resize(len);
			SendMessage(listClients, LB_GETTEXT, i, (LPARAM)&userKick[0]);
			if (userKick.compare(user) == 0)
			{
				MessageBox(hMainWind, _T("One does not simply kick ones self"), _T("ERROR"), MB_ICONERROR);
				break;
			}

			client->SendMsg(userKick, TYPE_ADMIN, MSG_ADMIN_KICK);
			break;
		}

		case ID_WHITEBOARD_START:
		{
			DialogBox(hInst, MAKEINTRESOURCE(WHITEBOARD_SETTINGS),hWnd, WBSettingsProc);
			break;
		}

		case ID_WHITEBOARD_TERMINATE:
		{
			client->SendMsg(TYPE_WHITEBOARD, MSG_WHITEBOARD_TERMINATE);
			break;
		}

		case ID_WHITEBOARD_INVITE:
		{
			DialogBox(hInst, MAKEINTRESOURCE(WHITEBOARD_INVITE), hWnd, WBInviteProc);
			break;
		}

		case ID_WHITEBOARD_KICK:
		{
			const UINT i = SendMessage(listClients, LB_GETCURSEL, 0, 0);
			const UINT len = SendMessage(listClients, LB_GETTEXTLEN, i, 0);
			std::tstring userKick;

			userKick.resize(len);
			SendMessage(listClients, LB_GETTEXT, i, (LPARAM)&userKick[0]);
			if(userKick.compare(user) == 0)
			{
				MessageBox(hMainWind, _T("One does not simply kick ones self"), _T("ERROR"), MB_ICONERROR);
				break;
			}

			client->SendMsg(userKick, TYPE_WHITEBOARD, MSG_WHITEBOARD_KICK);
			break;
		}


		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	}
	case WM_CREATE:
	{
		main = CreateMenu();

		file = CreateMenu();
		AppendMenu(main, MF_POPUP, (UINT_PTR)file, _T("File"));
		AppendMenu(file, MF_STRING, ID_SERV_CONNECT, _T("Connect"));
		AppendMenu(file, MF_STRING | MF_GRAYED, ID_SERV_DISCONNECT, _T("Disconnect"));
		AppendMenu(file, MF_STRING, ID_SERV_MANAGE, _T("Manage"));

		options = CreateMenu();
		AppendMenu(main, MF_POPUP, (UINT_PTR)options, _T("Options"));
		AppendMenu(options, MF_STRING, ID_VIEW_OPTIONS, _T("Options"));
		AppendMenu(options, MF_STRING, ID_VIEW_LOGS, _T("Logs"));

		wbMenu = CreateMenu();
		AppendMenu(main, MF_POPUP, (UINT_PTR)wbMenu, _T("Whiteboard"));
		AppendMenu(wbMenu, MF_STRING, ID_WHITEBOARD_START, _T("Start"));
		AppendMenu(wbMenu, MF_STRING | MF_GRAYED, ID_WHITEBOARD_TERMINATE, _T("Terminate"));

		SetMenu(hWnd, main);


		textDisp = CreateWindow(WC_EDIT, NULL, WS_CHILD | WS_VISIBLE | ES_MULTILINE | WS_BORDER | ES_READONLY | WS_VSCROLL, 0, 0, left, top, hWnd, (HMENU)ID_TEXTBOX_DISPLAY, hInst, 0);
		textInput = CreateWindow(WC_EDIT, NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | WS_VSCROLL, 0, top, left, topLen, hWnd, (HMENU)ID_TEXTBOX_INPUT, hInst, NULL);
		listClients = CreateWindow(WC_LISTBOX, NULL, WS_CHILD | WS_VISIBLE | LBS_STANDARD, left, 0, leftLen, top, hWnd, (HMENU)ID_LISTBOX_CLIENTS, hInst, NULL);

		buttonEnter = CreateWindow(WC_BUTTON, _T("Enter"), WS_CHILD | WS_VISIBLE | WS_DISABLED, left, top, leftLen, topLen, hWnd, (HMENU)ID_BUTTON_ENTER, hInst, NULL);

		for (UINT i = 0; i < nDialogs; i++)
		{
			psp[i] = { sizeof(PROPSHEETPAGE) };
			psp[i].dwFlags = PSP_DEFAULT;
		}
		psp[0].pszTemplate = MAKEINTRESOURCE(TAB_GENERAL);
		psp[0].pfnDlgProc = Opt_GeneralProc;

		psp[1].pszTemplate = MAKEINTRESOURCE(TAB_TEXT);
		psp[1].pfnDlgProc = Opt_TextProc;

		psp[2].pszTemplate = MAKEINTRESOURCE(TAB_FILES);
		psp[2].pfnDlgProc = Opt_FilesProc;

		break;
	}

	case WM_CREATEWIN:
	{
		switch( wParam )
		{
		case ID_WB:
		{
			WBParams *wbp = (WBParams*)lParam;
			CreateWBWindow( wbp->width, wbp->height );
			pWhiteboard->Initialize(wbHandle);
		}	break;
		}
	}	break;

	case WM_SIZE:
	{
		RecalcSizeVars(LOWORD(lParam), HIWORD(lParam));
		MoveWindow(textDisp, 0, 0, left, top, TRUE);
		MoveWindow(textInput, 0, top, left, topLen, TRUE);
		MoveWindow(listClients, left, 0, leftLen, top, TRUE);
		MoveWindow(buttonEnter, left, top, leftLen, topLen, TRUE);

		break;
	}

	case WM_PARENTNOTIFY:
		switch (LOWORD(wParam))
		{
			case WM_RBUTTONDOWN:
			{
				POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
				if (ChildWindowFromPoint(hWnd, pt) == listClients)
				{
					ClientToScreen(hWnd, &pt);
					ScreenToClient(listClients, &pt);
					UINT item = SendMessage(listClients, LB_ITEMFROMPOINT, 0, MAKELPARAM(pt.x, pt.y));
					if (!HIWORD(item) && LOWORD(item) != LB_ERR)
					{
						SendMessage(listClients, LB_SETCURSEL, item, 0);
						HMENU hPop = CreatePopupMenu();
						HMENU send = CreateMenu();
						HMENU admin = CreateMenu();
						HMENU whiteboard = CreateMenu();

						AppendMenu(admin, MF_STRING, ID_KICK, _T("Kick"));

						AppendMenu(send, MF_STRING, ID_SEND_FILE, _T("Send File"));
						AppendMenu(send, MF_STRING, ID_SEND_FOLDER, _T("Send Folder"));

						AppendMenu(whiteboard, MF_STRING, ID_WHITEBOARD_INVITE, _T("Invite"));
						AppendMenu(whiteboard, MF_STRING, ID_WHITEBOARD_KICK, _T("Kick"));

						AppendMenu(hPop, MF_POPUP, (UINT_PTR)admin, _T("Admin"));
						AppendMenu(hPop, MF_POPUP | (fileSend->Running() ? MF_GRAYED : MF_ENABLED), (UINT_PTR)send, _T("Send"));
						AppendMenu(hPop, MF_POPUP | (pWhiteboard == nullptr ? MF_GRAYED : MF_ENABLED), (UINT_PTR)whiteboard, _T("Whiteboard"));

						ClientToScreen(listClients, &pt);
						TrackPopupMenu(hPop, 0, pt.x, pt.y, 0, hWnd, NULL);
						DestroyMenu(hPop);
					}
				}
			} // case WM_RBUTTONDOWN
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
			
		}
		return 0;
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
			SetFocus(textInput);
		break;

	case WM_DESTROY:
		CoUninitialize();
		DestroyClient(client);
		CleanupNetworking();
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

LRESULT CALLBACK WbProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static POINT pt, prevPt;

	switch (message)
	{
	/*case WM_INPUT:
	{
		UINT size;
		GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER));

		RAWINPUT *ri = (RAWINPUT*)alloc<BYTE>(size);

		GetRawInputData((HRAWINPUT)lParam, RID_INPUT, ri, &size, sizeof(RAWINPUTHEADER));

		if(ri->header.dwType == RIM_TYPEMOUSE)
		{
			RAWMOUSE& mouse = ri->data.mouse;

			if(mouse.usFlags == MOUSE_MOVE_RELATIVE)
			{
				prevPt = pt;
				mouseX += mouseSpeed * (double)mouse.lLastX;
				mouseY += mouseSpeed * (double)mouse.lLastY;
				pt = { mouseX, mouseY };
			}

			else if(mouse.usFlags & MOUSE_MOVE_ABSOLUTE)
			{
				prevPt = pt;
				pt = { mouseX = mouse.lLastX, mouseY = mouse.lLastY };
			}

			RECT rc{};
			GetClientRect(hWnd, &rc);
			if(PtInRect(&rc, pt))
			{
				POINT temp = pt;
				ScreenToClient(hWnd, &temp);

				SetCursorPos(temp.x, temp.y);

				switch(mouse.usButtonFlags)
				{
				case RI_MOUSE_LEFT_BUTTON_DOWN:
					mServ.OnLeftPressed(temp.x, temp.y);
					break;

				case RI_MOUSE_LEFT_BUTTON_UP:
					mServ.OnLeftReleased(temp.x, temp.y);
					break;

				case RI_MOUSE_RIGHT_BUTTON_DOWN:
					mServ.OnRightPressed(temp.x, temp.y);
					break;

				case RI_MOUSE_RIGHT_BUTTON_UP:
					mServ.OnRightReleased(temp.x, temp.y);
					break;
				}

				if((pt.x != prevPt.x) || (pt.y != prevPt.y))
					mServ.OnMouseMove(temp.x, temp.y);
			}
		}

		LIB_TCHAR buffer[128];
		_stprintf(buffer, _T("(%d, %d)"), pt.x, pt.y);
		SendMessage(textDisp, WM_SETTEXT, 0, (LPARAM)buffer);
		dealloc(ri);
		break;
	}*/


	//	if(ri->header.dwType == RIM_TYPEMOUSE)
	//	{
	//		RAWMOUSE& mouse = ri->data.mouse;

	//		if(mouse.usFlags == MOUSE_MOVE_RELATIVE)
	//		{
	//			prevPt = pt;
	//			pt = { pt.x + mouse.lLastX, pt.y + mouse.lLastY };
	//		}

	//		else if(mouse.usFlags & MOUSE_MOVE_ABSOLUTE)
	//		{
	//			prevPt = pt;
	//			pt = { mouse.lLastX, mouse.lLastY };
	//		}

	//		RECT rc{};
	//		GetClientRect(hWnd, &rc);
	//		if(PtInRect(&rc, pt))
	//		{
	//			POINT temp = pt;
	//			ScreenToClient(hWnd, &temp);

	//			switch(mouse.usButtonFlags)
	//			{
	//			case RI_MOUSE_LEFT_BUTTON_DOWN:
	//				mServ.OnLeftPressed(temp.x, temp.y);
	//				break;

	//			case RI_MOUSE_LEFT_BUTTON_UP:
	//				mServ.OnLeftReleased(temp.x, temp.y);
	//				break;

	//			case RI_MOUSE_RIGHT_BUTTON_DOWN:
	//				mServ.OnRightPressed(temp.x, temp.y);
	//				break;

	//			case RI_MOUSE_RIGHT_BUTTON_UP:
	//				mServ.OnRightReleased(temp.x, temp.y);
	//				break;
	//			}

	//			if((pt.x != prevPt.x) || (pt.y != prevPt.y))
	//				mServ.OnMouseMove(temp.x, temp.y);
	//		}
	//	}

	//	dealloc(ri);
	//	break;
	//}
	
	//case WM_MOUSEMOVE:
	//	mServ.OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
	//	break;
	//case WM_LBUTTONDOWN:
	//	mServ.OnLeftPressed(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
	//	break;
	//case WM_RBUTTONDOWN:
	//	mServ.OnRightPressed(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
	//	break;
	//case WM_LBUTTONUP:
	//	mServ.OnLeftReleased(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
	//	break;
	//case WM_RBUTTONUP:
	//	mServ.OnRightReleased(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
	//	break;


	case WM_CREATE:
	{
		RAWINPUTDEVICE rid;
		rid.usUsagePage = 0x01;
		rid.usUsage = 0x02;
		rid.dwFlags = NULL;
		rid.hwndTarget = hWnd;

		RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE));

		const double exponent = pow(4, 1.0 / 10.0);

		int temp;
		SystemParametersInfo(SPI_GETMOUSESPEED, 0, &temp, 0);

		if(temp >= 10)
		{
			const double exponent = pow(4, 1.0 / 10.0);
			mouseSpeed = pow(exponent, temp - 10);
		}
		else if(temp < 10)
		{
			const double exponent = pow((1.0 / 10.0) / (4.0 * (9.0 / 10.0)), 1.0 / 10.0);
			mouseSpeed = pow(exponent, 10 - temp);
		};


		//mouseThread = CreateThread(NULL, NULL, Mouse, NULL, NULL, &mouseThreadID);

		break;
	}

	case WM_ACTIVATE:
	{
		GetCursorPos(&pt);
		mouseX = pt.x;
		mouseY = pt.y;

		LIB_TCHAR buffer[128];
		_stprintf(buffer, _T("(%d, %d)"), pt.x, pt.y);
		SendMessage(textDisp, WM_SETTEXT, 0, (LPARAM)buffer);
		break;
	}

	case WM_CLOSE:
		client->SendMsg(TYPE_WHITEBOARD, MSG_WHITEBOARD_LEFT);
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		EnableMenuItem(wbMenu, ID_WHITEBOARD_START, MF_ENABLED);
		EnableMenuItem(wbMenu, ID_WHITEBOARD_TERMINATE, MF_GRAYED);

		/*PostThreadMessage(mouseThreadID, WM_QUIT, 0, 0);
		CloseHandle(mouseThread);*/

		wbHandle = nullptr;
		UnregisterClass(wbClassName, hInst);
		destruct(pWhiteboard);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

INT_PTR CALLBACK ConnectProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND list;
	switch (message)
	{
	case WM_COMMAND:
	{
		const short id = LOWORD(wParam);
		switch (id)
		{
		case ID_ADDR_LIST_CONNECT:
			if (HIWORD(wParam) == LBN_SELCHANGE && SendMessage(list, LB_GETCURSEL, 0, 0) != LB_ERR)
			{
				EnableWindow(GetDlgItem(hWnd, IDOK), true);
			}
			break;
		case IDOK:
		{
			const UINT index = SendMessage(list, LB_GETCURSEL, 0, 0);
			const UINT len = SendMessage(list, LB_GETTEXTLEN, index, 0);
			std::tstring str;
			str.resize(len + 1);
			SendMessage(list, LB_GETTEXT, index, (LPARAM)&str[0]);
			const UINT pos = str.find(_T(":"));
			Connect(str.substr(0, pos).c_str(), str.substr(pos + 1).c_str(), timeOut);
			if (client->IsConnected())
			{
				client->RecvServData();
				MsgStreamWriter streamWriter(TYPE_VERSION, MSG_VERSION_CHECK, sizeof(float));
				streamWriter.Write(APPVERSION);

				HANDLE hnd = client->SendServData(streamWriter, streamWriter.GetSize());
				WaitAndCloseHandle(hnd);

				EndDialog(hWnd, id);
			}
			else
			{
				MessageBox(hWnd, _T("Unable to connect to server"), _T("ERROR"), MB_ICONERROR);
			}
			break;
		}
		case IDCANCEL:
			EndDialog(hWnd, id);
			break;
		}
		break;
	}
	case WM_INITDIALOG:
	{
		list = GetDlgItem(hWnd, ID_ADDR_LIST_CONNECT);
		for (auto& i : servList)
			SendMessage(list, LB_ADDSTRING, 0, (LPARAM)i.c_str());
		return 1;
	}
	}
	return 0;
}

INT_PTR CALLBACK ManageProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND list, ipInput, portInput, add, remove;
	switch (message)
	{
	case WM_COMMAND:
	{
		const short id = LOWORD(wParam);
		switch (id)
		{
		case ID_MANAGE_PORT:
		case ID_IP_ADD:
			switch (HIWORD(wParam))
			{
			case EN_CHANGE:
				if((SendMessage(list, IPM_ISBLANK, 0, 0) == 0) && (SendMessage(portInput, WM_GETTEXTLENGTH, 0, 0) > 0))
					EnableWindow(add, TRUE);
				else
					EnableWindow(add, FALSE);
				break;
			}
			break;
		case ID_ADDR_LIST_MANAGE:
		{
			if (SendMessage(list, LB_GETCURSEL, 0, 0) != LB_ERR)
				EnableWindow(remove, TRUE);
			else
				EnableWindow(remove, FALSE);
			break;
		}
		case ID_BUTTON_REMOVE:
		{
			const UINT index = SendMessage(list, LB_GETCURSEL, 0, 0);
			const UINT len = SendMessage(list, LB_GETTEXTLEN, index, 0);
			LIB_TCHAR* buffer = alloc<LIB_TCHAR>(len + 1);
			SendMessage(list, LB_GETTEXT, index, (LPARAM)buffer);
			SendMessage(list, LB_DELETESTRING, index, 0);
			servList.remove(buffer);
			dealloc(buffer);

			EnableWindow(remove, FALSE);
			break;
		}

		case ID_BUTTON_ADD:
		{
			DWORD ip = 0;
			SendMessage(ipInput, IPM_GETADDRESS, 0, (LPARAM)&ip);
			LIB_TCHAR buffer[INET_ADDRSTRLEN] = {};
			_stprintf(buffer, _T("%d.%d.%d.%d:%d"), FIRST_IPADDRESS(ip), SECOND_IPADDRESS(ip), THIRD_IPADDRESS(ip), FOURTH_IPADDRESS(ip), GetDlgItemInt(hWnd,ID_MANAGE_PORT,NULL,FALSE));
			SendMessage(ipInput, IPM_CLEARADDRESS, 0, 0);
			servList.push_back(buffer);
			SendMessage(list, LB_ADDSTRING, 0, (LPARAM)buffer);

			EnableWindow(add, FALSE);
			break;
		}


		case IDOK:
			SaveServList();
			EndDialog(hWnd, id);
			break;
		case IDCANCEL:
			EndDialog(hWnd, id);
			break;
		}
		break;
	}
	case WM_INITDIALOG:
	{
		list = GetDlgItem(hWnd, ID_ADDR_LIST_MANAGE), ipInput = GetDlgItem(hWnd, ID_IP_ADD), portInput = GetDlgItem(hWnd, ID_MANAGE_PORT), add = GetDlgItem(hWnd, ID_BUTTON_ADD), remove = GetDlgItem(hWnd, ID_BUTTON_REMOVE);
		
		SendMessage(portInput, EM_SETLIMITTEXT, 5, 0);
		SetDlgItemInt(hWnd, ID_MANAGE_PORT, DEFAULTPORT, FALSE);

		for (auto& i : servList)
			SendMessage(list, LB_ADDSTRING, 0, (LPARAM)i.c_str());

		EnableWindow(add, FALSE);
		EnableWindow(remove, FALSE);
		return 1;
	}
	}
	return 0;
}

DWORD CALLBACK InvalidUserPass(LPVOID hWnd)
{
	Disconnect();
	EndDialog((HWND)hWnd, 0);
	MessageBox(hMainWind, _T("Error invalid username/password"), _T("ERROR"), MB_ICONERROR);
	return 0;
}

INT_PTR CALLBACK AuthenticateProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND textUsername, textPassword;
	switch (message)
	{
	case WM_COMMAND:
	{
		const short id = LOWORD(wParam);
		switch (id)
		{
		case ID_AUTH_USERNAME:
		case ID_AUTH_PASSWORD:
		{
			if (SendMessage(textUsername, WM_GETTEXTLENGTH, 0, 0) > 0)
			{
				if (SendMessage(textPassword, WM_GETTEXTLENGTH, 0, 0) > 0)
				{
					EnableWindow(GetDlgItem(hWnd, IDOK), true);
					break;
				}
			}
			EnableWindow(GetDlgItem(hWnd, IDOK), false);
			break;
		}

		case IDOK:
		{
			const UINT userLen = SendMessage(textUsername, WM_GETTEXTLENGTH, 0, 0) + 1;
			user.resize(userLen);
			SendMessage(textUsername, WM_GETTEXT, userLen, (LPARAM)&user[0]);
			user.pop_back();

			const UINT passLen = SendMessage(textPassword, WM_GETTEXTLENGTH, 0, 0) + 1;
			std::tstring pass;
			pass.resize(passLen);
			SendMessage(textPassword, WM_GETTEXT, passLen, (LPARAM)&pass[0]);
			pass.pop_back();

			if ((user.find_first_of(_T(":")) != std::tstring::npos) || (pass.find_first_of(_T(":")) != std::tstring::npos) || (user.back() == ' '))
			{
				CloseHandle(CreateThread(NULL, 0, InvalidUserPass, (LPVOID)hWnd, NULL, NULL));
				break;
			}

			std::tstring send = user + _T(":") + pass;
			const UINT sendLen = send.size() + 1;
			MsgStreamWriter streamWriter(TYPE_REQUEST, MSG_REQUEST_AUTHENTICATION, sendLen * sizeof(LIB_TCHAR));
			streamWriter.Write(send.c_str(), sendLen);

			HANDLE hnd = client->SendServData(streamWriter, streamWriter.GetSize());
			WaitAndCloseHandle(hnd);

			EndDialog(hWnd, id);
			break;
		}
		case IDCANCEL:
			Disconnect();
			EndDialog(hWnd, id);
			break;
		}
		break;
	}
	case WM_INITDIALOG:
	{
		textUsername = GetDlgItem(hWnd, ID_AUTH_USERNAME), textPassword = GetDlgItem(hWnd, ID_AUTH_PASSWORD);
		SendMessage(textUsername, EM_SETLIMITTEXT, maxUserLen, 0);
		SetFocus(textUsername);
		return 0;
	}
	}
	return 0;
}

INT_PTR CALLBACK Opt_GeneralProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND flashCount;
	switch (message)
	{
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case ID_FLASH_TASKBAR:
		{
			EnableWindow(flashCount, (BST_CHECKED == IsDlgButtonChecked(hWnd, ID_FLASH_TASKBAR)));
			break;
		}
		}
		break;
	}
	case WM_NOTIFY:
	{
		switch (((LPNMHDR)lParam)->code)
		{
		case PSN_SETACTIVE:
		{
			CheckDlgButton(hWnd, ID_TIMESTAMPS, opts->TimeStamps() ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hWnd, ID_STARTUP, opts->AutoStartup() ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hWnd, ID_FLASH_TASKBAR, opts->FlashTaskbar(hMainWind) ? BST_CHECKED : BST_UNCHECKED);
			
			SendMessage(flashCount, WM_SETTEXT, 0, (LPARAM)To_String(opts->GetFlashCount()).c_str());
			EnableWindow(flashCount, opts->FlashTaskbar(hMainWind));

			SetWindowLongPtr(hWnd, DWL_MSGRESULT, PSNRET_NOERROR);
			break;
		}
		case PSN_APPLY:
		{
			LIB_TCHAR buffer[4];
			SendMessage(flashCount, WM_GETTEXT, 4, (LPARAM)buffer);

			opts->SetGeneral(
				(BST_CHECKED == IsDlgButtonChecked(hWnd, ID_TIMESTAMPS)), 
				(BST_CHECKED == IsDlgButtonChecked(hWnd, ID_STARTUP)), 
				(BST_CHECKED == IsDlgButtonChecked(hWnd, ID_FLASH_TASKBAR)), 
				(UCHAR)abs(_tstoi(buffer)));
			
			opts->Save(windowName);

			SetWindowLongPtr(hWnd, DWL_MSGRESULT, PSNRET_NOERROR);
			break;
		}
		case PSN_RESET:
		{
			SetWindowLongPtr(hWnd, DWL_MSGRESULT, PSNRET_NOERROR);
			break;
		}
		}
		break;
	}
	case WM_INITDIALOG:
	{
		flashCount = GetDlgItem(hWnd, ID_FLASH_COUNT);

		return 1;
	}
	}
	return 0;
}

INT_PTR CALLBACK Opt_TextProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND fontDisp;
	switch (message)
	{
	case WM_COMMAND:
	{
		const short id = LOWORD(wParam);
		switch (id)
		{
		case ID_FONT_BUTTON:
		{
			HFONT font;
			COLORREF color;
			if (FileMisc::BrowseFont(hWnd, font, color))
			{
				
			}
			break;
		}
		}
		break;
	}
	case WM_NOTIFY:
	{
		switch (((LPNMHDR)lParam)->code)
		{
		case PSN_SETACTIVE:
		{
			SendMessage(fontDisp, WM_SETTEXT, 0, (LPARAM)_T(""));
			SetWindowLongPtr(hWnd, DWL_MSGRESULT, PSNRET_NOERROR);
			break;
		}
		case PSN_APPLY:
		{
			SetWindowLong(hWnd, DWL_MSGRESULT, PSNRET_NOERROR);
			break;
		}
		case PSN_RESET:
		{
			SetWindowLong(hWnd, DWL_MSGRESULT, PSNRET_NOERROR);
			break;
		}
		break;
		}
		break;
	}
	case WM_INITDIALOG:
	{
		fontDisp = GetDlgItem(hWnd, ID_FONT_TEXT);
		return 1;
	}
	}
	return 0;
}

INT_PTR CALLBACK Opt_FilesProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND pathDisp;
	switch (message)
	{
	case WM_COMMAND:
	{
		const short id = LOWORD(wParam);
		switch (id)
		{
		case ID_PATH_BUTTON:
		{
			LIB_TCHAR buffer[MAX_PATH] = {};
			if (FileMisc::BrowseFolder(_T("Browse directories"), buffer, hWnd))
			{
				opts->SetDownloadPath(std::tstring(buffer));
				SendMessage(pathDisp, WM_SETTEXT, 0, (LPARAM)opts->GetDownloadPath().c_str());
			}
			break;
		}
		}
		break;
	}

	case WM_NOTIFY:
	{
		switch (((LPNMHDR)lParam)->code)
		{
		case PSN_SETACTIVE:
		{
			SendMessage(pathDisp, WM_SETTEXT, 0, (LPARAM)opts->GetDownloadPath().c_str());
			SetWindowLongPtr(hWnd, DWL_MSGRESULT, PSNRET_NOERROR);
			break;
		}
		case PSN_APPLY:
		{
			SetWindowLong(hWnd, DWL_MSGRESULT, PSNRET_NOERROR);
			break;
		}
		case PSN_RESET:
		{
			SetWindowLong(hWnd, DWL_MSGRESULT, PSNRET_NOERROR);
			break;
		}
		break;
		}
		break;
	}
	case WM_INITDIALOG:
	{
		pathDisp = GetDlgItem(hWnd, ID_PATH_TEXT);
		return 1;
	}
	}
	return 0;
}

INT_PTR CALLBACK RequestFileProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND text;
	switch (message)
	{
	case WM_COMMAND:
	{
		const short id = LOWORD(wParam);
		switch (id)
		{
		case IDOK:
			client->SendMsg(fileReceive->GetUser(), TYPE_RESPONSE, MSG_RESPONSE_TRANSFER_CONFIRMED);
			EndDialog(hWnd, id);
			break;

		case IDCANCEL:
			client->SendMsg(fileReceive->GetUser(), TYPE_RESPONSE, MSG_RESPONSE_TRANSFER_DECLINED);
			EndDialog(hWnd, id);
			break;

		}
		break;
	}

	case WM_INITDIALOG:
	{
		SendMessage(GetDlgItem(hWnd, ID_REQUEST_TEXT), WM_SETTEXT, 0, lParam);
		return 1;
	}
	}
	return 0;
}

INT_PTR CALLBACK RequestWBProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND text;
	switch(message)
	{
	case WM_COMMAND:
	{
		const short id = LOWORD(wParam);
		switch(id)
		{
		case IDOK:
			client->SendMsg(TYPE_RESPONSE, MSG_RESPONSE_WHITEBOARD_CONFIRMED);
			EndDialog(hWnd, id);
			break;

		case IDCANCEL:
			client->SendMsg(TYPE_RESPONSE, MSG_RESPONSE_WHITEBOARD_DECLINED);
			EndDialog(hWnd, id);
			break;

		}
		break;
	}

	case WM_INITDIALOG:
	{
		SendMessage(GetDlgItem(hWnd, ID_REQUEST_TEXT), WM_SETTEXT, 0, lParam);

		if(pWhiteboard)
		{
			client->SendMsg(TYPE_RESPONSE, MSG_RESPONSE_WHITEBOARD_DECLINED);
			EndDialog(hWnd, 0);
		}

		return 0;
	}
	}
	return 0;
}

INT_PTR CALLBACK WBSettingsProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND X, Y, FPS, Colors;
	switch(message)
	{
	case WM_COMMAND:
	{
		const short id = LOWORD(wParam);
		switch(id)
		{
		case IDOK:
		{
			MsgStreamWriter streamWriter(TYPE_WHITEBOARD, MSG_WHITEBOARD_SETTINGS, sizeof(WBParams));
			streamWriter.Write(WBParams(
				GetDlgItemInt(hWnd, WHITEBOARD_RES_X, NULL, FALSE), 
				GetDlgItemInt(hWnd, WHITEBOARD_RES_Y, NULL, FALSE),
				GetDlgItemInt(hWnd, WHITEBOARD_FPS, NULL, FALSE), 
				ComboBox_GetCurSel(Colors)));

			HANDLE hnd = client->SendServData(streamWriter, streamWriter.GetSize());
			WaitAndCloseHandle(hnd);

			EnableMenuItem(wbMenu, ID_WHITEBOARD_START, MF_GRAYED);
			EnableMenuItem(wbMenu, ID_WHITEBOARD_TERMINATE, MF_ENABLED);

			EndDialog(hWnd, id);
			break;
		}
		case IDCANCEL:
		{
			EndDialog(hWnd, id);
			break;
		}
		}
		break;
	}

	case WM_DESTROY:
		ImageList_Destroy( (HIMAGELIST)SendMessage( Colors, CBEM_GETIMAGELIST, 0, 0 ) );
		break;

	case WM_INITDIALOG:
	{
		X = GetDlgItem(hWnd, WHITEBOARD_RES_X), 
			Y = GetDlgItem(hWnd, WHITEBOARD_RES_Y), 
			FPS = GetDlgItem(hWnd, WHITEBOARD_FPS), 
			Colors = GetDlgItem(hWnd, WHITEBOARD_COLORSEL);
		SendMessage(X, EM_SETLIMITTEXT, 4, 0);
		SendMessage(Y, EM_SETLIMITTEXT, 4, 0);
		SendMessage(FPS, EM_SETLIMITTEXT, 3, 0);
		
		SetDlgItemInt(hWnd, WHITEBOARD_RES_X, WB_DEF_RES_X, FALSE);
		SetDlgItemInt(hWnd, WHITEBOARD_RES_Y, WB_DEF_RES_Y, FALSE);
		SetDlgItemInt(hWnd, WHITEBOARD_FPS, WB_DEF_FPS, FALSE);

		BYTE count;
		palette.Get(count);

		const UINT iLen = 50, iHeight = 16, nPixels = iLen * iHeight;
		HIMAGELIST himl = ImageList_Create(iLen, iHeight, ILC_COLOR32, count, 0);
		D3DCOLOR* pBits = alloc<D3DCOLOR>( nPixels );

		for(BYTE p = 0; p < count; p++)
		{
			for(UINT i = 0; i < nPixels; i++)
				pBits[i] = palette.GetRGBColor(p);
			HBITMAP hbm = CreateBitmap( iLen, iHeight, 1, 32, pBits );
			ImageList_Add( himl, hbm, NULL );
			DeleteObject( hbm );

			COMBOBOXEXITEM cbei{};
			cbei.iItem = -1;
			cbei.mask = CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
			cbei.iImage = cbei.iSelectedImage = p;
			SendMessage( Colors, CBEM_INSERTITEM, 0, (LPARAM)&cbei );
		}

		dealloc( pBits );

		SendDlgItemMessage( hWnd, WHITEBOARD_COLORSEL, CBEM_SETIMAGELIST, 0, (LPARAM)himl );
		ComboBox_SetCurSel( Colors, 0 );
		SendDlgItemMessage(hWnd, -1, CB_SETMINVISIBLE, 10, 0);

		SetFocus(X);
		return 0;
	}
	}
	return 0;
}

INT_PTR CALLBACK WBInviteProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND draw, invite;
	static std::tstring usersel;
	static UINT len;
	switch(message)
	{
	case WM_COMMAND:
	{
		const short id = LOWORD(wParam);
		switch(id)
		{
		case IDOK:
		{
			//const bool canInvite = (BST_CHECKED == IsDlgButtonChecked(hWnd, ID_WHITEBOARD_CANINVITE));
			//const bool canDraw = (BST_CHECKED == IsDlgButtonChecked(hWnd, ID_WHITEBOARD_CANDRAW));

			MsgStreamWriter streamWriter(TYPE_REQUEST, MSG_REQUEST_WHITEBOARD, (len * sizeof(LIB_TCHAR)) + sizeof(bool));
			streamWriter.Write(usersel.c_str(), len);
			streamWriter.Write(BST_CHECKED == IsDlgButtonChecked(hWnd, ID_WHITEBOARD_CANDRAW));

			HANDLE hnd = client->SendServData(streamWriter, streamWriter.GetSize());
			WaitAndCloseHandle(hnd);

			EndDialog(hWnd, id);
			break;
		}
		case IDCANCEL:
		{
			EndDialog(hWnd, id);
			break;
		}
		}
		break;
	}

	case WM_INITDIALOG:
	{
		const UINT i = SendMessage(listClients, LB_GETCURSEL, 0, 0);
		len = SendMessage(listClients, LB_GETTEXTLEN, i, 0) + 1;
		usersel.resize(len);
		SendMessage(listClients, LB_GETTEXT, i, (LPARAM)&usersel[0]);
		usersel.pop_back();

		if(usersel.compare(user) == 0)
		{
			MessageBox(hMainWind, _T("One does not simply invite themself to the party"), _T("ERROR"), MB_ICONERROR);
			EndDialog(hWnd, 0);
		}
		draw = GetDlgItem(hWnd, ID_WHITEBOARD_CANDRAW), invite = GetDlgItem(hWnd, ID_WHITEBOARD_CANINVITE);
		CheckDlgButton(hWnd, ID_WHITEBOARD_CANDRAW, BST_CHECKED);
		//CheckDlgButton(hWnd, ID_WHITEBOARD_CANINVITE, BST_CHECKED);

		return 0;
	}
	}
	return 0;
}
