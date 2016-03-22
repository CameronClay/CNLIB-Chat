// Client version of Windows.cpp
#include "StdAfx.h"
#include "resource.h"
//#include "CNLIB/Typedefs.h"
#include "CNLIB/TCPClientInterface.h"
#include "CNLIB/MsgStream.h"
//#include "CNLIB/File.h"
#include "CNLIB/HeapAlloc.h"
#include "MessagesExt.h"
#include "Format.h"
#include "TextDisplay.h"
#include "Options.h"
#include "FileTransfer.h"
#include "Palette.h"
#include "Whiteboard.h"
#include "DebugHelper.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "TCPCS.lib")


const float APPVERSION = 0.35f;
const float CONFIGVERSION = .0025f;
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

#define ID_SEND_FILES 9
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
#define WB_DEF_FPS 20

#define WM_CREATEWIN WM_APP
#define ID_WB 1

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WbProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK LogsProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK LogReaderProc(HWND, UINT, WPARAM, LPARAM);
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
bool wbCanDraw = false;
// Whiteboard declarations end

static HWND textDisp, textInput, listClients;
static HMENU main, file, options;

HANDLE wbPThread;
DWORD wbPThreadID;

TextDisplay textBuffer;

std::tstring user;
#pragma endregion

HWND CreateWBWindow(USHORT width, USHORT height);

DWORD CALLBACK WBPThread(LPVOID param)
{
	WBParams* wbp = (WBParams*)param;
	CreateWBWindow(wbp->width, wbp->height);
	destruct(wbp);

	HACCEL hndAccel = LoadAccelerators(hInst, MAKEINTRESOURCE(WBHKS));

	MSG msg = {};
	while(GetMessage(&msg, NULL, 0, 0))
	{
		if(!TranslateAccelerator(wbHandle, hndAccel, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}

HWND CreateWBWindow(USHORT width, USHORT height)
{
	if(!wbAtom)
	{
		WNDCLASS wc = {};
		wc.lpfnWndProc = WbProc;
		wc.hInstance = hInst;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.lpszClassName = wbClassName;

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

		DWORD style =
			//WS_CHILD |			// Child window that
			WS_POPUPWINDOW |	// Is not contained in parent window
			WS_CAPTION |		// Shows the title bar
			WS_MINIMIZEBOX;		// Shows the minimize button

		AdjustWindowRect(&wr, style, FALSE);
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
	left = USHORT(screenWidth * (8.75f / 10.0f)), leftLen = (screenWidth - left) - 10;
	top = USHORT(screenHeight * (8.0f / 10.0f)), topLen = screenHeight - top;
}

void Connect(const LIB_TCHAR* dest, const LIB_TCHAR* port, bool ipv6, float timeOut)
{
	if (client->Connect(dest, port, ipv6, timeOut))
	{
		EnableMenuItem(file, ID_SERV_CONNECT, MF_GRAYED);
		EnableMenuItem(file, ID_SERV_DISCONNECT, MF_ENABLED);
	}
}

void ClearAll()
{
	//SendMessage(textDisp, WM_SETTEXT, 0, (LPARAM)_T(""));
	PostMessage(textInput, WM_SETTEXT, 0, (LPARAM)_T(""));
	PostMessage(listClients, LB_RESETCONTENT, 0, (LPARAM)_T(""));
}

void Disconnect()
{
	client->Disconnect();
	PostMessage(listClients, LB_RESETCONTENT, 0, (LPARAM)_T(""));
	EnableMenuItem(file, ID_SERV_CONNECT, MF_ENABLED);
	EnableMenuItem(file, ID_SERV_DISCONNECT, MF_GRAYED);
}

void Flash()
{
	// I put FLASHWINFO in Options
	opts->FlashTaskbar(hMainWind);
}

//returns index if found, -1 if not found
int FindClient(const std::tstring& name)
{
	const USHORT count = (USHORT)SendMessage(listClients, LB_GETCOUNT, 0, 0);
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

void MsgHandler(TCPClientInterface&, MsgStreamReader streamReader)
{
	char* dat = streamReader.GetData();
	const short type = streamReader.GetType(), msg = streamReader.GetMsg();
		
	switch (type)
	{
		case TYPE_CHANGE:
		{
			switch(msg)
			{
			case MSG_CHANGE_CONNECT:
			{
				const std::tstring name = streamReader.Read<std::tstring>();

				LIB_TCHAR buffer[128];
				_stprintf(buffer, _T("Server: %s has connected!"), name.c_str());

				std::tstring str;
				Format::FormatText(buffer, str, opts->TimeStamps());
				textBuffer.WriteLine(str);
				opts->WriteLine(str);

				if (FindClient(name) == -1)
					SendMessage(listClients, LB_ADDSTRING, 0, (LPARAM)name.c_str());

				Flash();
				break;
			}
			case MSG_CHANGE_CONNECTINIT:
			{
				const std::tstring name = streamReader.Read<std::tstring>();
				if (FindClient(name) == -1)
					SendMessage(listClients, LB_ADDSTRING, 0, (LPARAM)name.c_str());

				break;
			}
			case MSG_CHANGE_DISCONNECT:
			{
				const std::tstring name = streamReader.Read<std::tstring>();
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

				if ((item = FindClient(name)) != -1)
					SendMessage(listClients, LB_DELETESTRING, item, 0);

				LIB_TCHAR buffer[128];
				_stprintf(buffer, _T("Server: %s has disconnected!"), name.c_str());


				std::tstring str;
				Format::FormatText(buffer, str, opts->TimeStamps());
				textBuffer.WriteLine(str);
				opts->WriteLine(str);

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
					std::tstring str = streamReader.Read<std::tstring>();
					Format::FormatText(str, str, opts->TimeStamps());
					textBuffer.WriteLine(str);
					opts->WriteLine(str);

					Flash();
					break;
				}
				case MSG_DATA_BITMAP:
				{
					if(pWhiteboard)
					{
						RectU rect = streamReader.Read<RectU>();
						BYTE* pixels = streamReader.ReadEnd<BYTE>();
						pWhiteboard->Frame(rect, pixels);
					}
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
					_stprintf(buffer, _T("%s has declined your transfer request!"), streamReader.Read<std::tstring>().c_str());
					MessageBox(hMainWind, buffer, _T("DECLINED"), MB_ICONERROR);
					break;
				}

				case MSG_RESPONSE_TRANSFER_CONFIRMED:
				{
					LIB_TCHAR buffer[255];
					_stprintf(buffer, _T("%s has confirmed your transfer request!"), streamReader.Read<std::tstring>().c_str());
					MessageBox(hMainWind, buffer, _T("Success"), MB_OK);
					fileSend->StartSend();
					break;
				}

				case MSG_RESPONSE_WHITEBOARD_CONFIRMED:
				{
					LIB_TCHAR buffer[255];
					_stprintf(buffer, _T("%s has joined the Whiteboard!"), streamReader.Read<std::tstring>().c_str());
					MessageBox(hMainWind, buffer, _T("Success"), MB_OK);
					break;
				}

				case MSG_RESPONSE_WHITEBOARD_DECLINED:
				{
					LIB_TCHAR buffer[255];
					_stprintf(buffer, _T("%s has declined the whiteboard!"), streamReader.Read<std::tstring>().c_str());
					MessageBox(hMainWind, buffer, _T("DECLINED"), MB_ICONERROR);
					break;
				}

			}// MSG_RESPONSES
			break;
		}//TYPE_RESPONSE

		case TYPE_FILE:
		{
			switch (msg)
			{
				case MSG_FILE_LIST:
				{
					fileReceive->RecvFileNameList(streamReader, (std::tstring&)opts->GetDownloadPath());//temp cast because filetransfer stuff will be redone
					break;
				}
				case MSG_FILE_DATA:
				{
					fileReceive->RecvFile((BYTE*)dat, streamReader.GetDataSize());
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
					fileReceive->GetUser() = streamReader.Read<std::tstring>();
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
					wbCanDraw = streamReader.Read<bool>();
					std::tstring name = streamReader.Read<std::tstring>();

					LIB_TCHAR buffer[255];
					_stprintf(buffer, _T("%s wants to display a whiteboard. Can Draw: %s"), name.c_str(), wbCanDraw ? _T("Yes") : _T("No"));

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
					_stprintf(buffer, _T("%s has kicked you from the server!"), streamReader.Read<std::tstring>().c_str());
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

				WBParams pParams = streamReader.Read<WBParams>();

				pWhiteboard = construct<Whiteboard>(
					*client,
					palette,
					pParams.width,
					pParams.height,
					pParams.fps,
					pParams.clrIndex );

				SendMessage( hMainWind, WM_CREATEWIN, ID_WB, (LPARAM)&pParams );
				break;
			}
			case MSG_WHITEBOARD_TERMINATE:
			{
				// in case user declined
				if(IsWindow(wbHandle))
				{
					SetForegroundWindow(wbHandle);
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
				_stprintf(buffer, _T("%s has removed you from the whiteboard!"), streamReader.Read<std::tstring>().c_str());
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

void DisconnectHandler(bool unexpected) // for disconnection
{
	if(unexpected)
	{
		Flash();
		MessageBox(hMainWind, _T("You have been disconnected from server!"), _T("ERROR"), MB_ICONERROR);
	}

	ClearAll();
	EnableMenuItem(file, ID_SERV_CONNECT, MF_ENABLED);
	EnableMenuItem(file, ID_SERV_DISCONNECT, MF_GRAYED);
}

void SaveServList()
{
	File file(servListFilePath, FILE_GENERIC_WRITE, FILE_ATTRIBUTE_NORMAL, CREATE_ALWAYS);
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
	FileMisc::CreateFolder(folderName);
	_stprintf(folderPath, _T("%s\\%s"), folderPath, folderName);
	FileMisc::SetCurDirectory(folderPath);

	_stprintf(servListFilePath, _T("%s\\%s"), folderPath, servListFileName);
	File file(servListFilePath, FILE_GENERIC_READ);

	std::tstring temp;
	while(file.ReadString(temp))
	{
		servList.push_back(temp);
	}
	file.Close();


	_stprintf(optionsFilePath, _T("%s\\%s"), folderPath, optionsFileName);

	InitializeNetworking();

	client = CreateClient(&MsgHandler, &DisconnectHandler);
	fileSend = uqpc<FileSend>(construct<FileSend>(*client, hMainWind, &SendFinishedHandler, &SendCanceledHandler));
	fileReceive = uqpc<FileReceive>(construct<FileReceive>(*client, hMainWind, &ReceiveFinishedHandler, &ReceiveCanceledHandler));
	opts = uqpc<Options>(construct<Options>(std::tstring(optionsFilePath), CONFIGVERSION));

	opts->Load(windowName);
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR, int nCmdShow)
{
	WinMainInit();

	HRESULT res = CoInitialize(NULL);
	assert(SUCCEEDED(res));

	WNDCLASSEX wcex{};

	wcex.cbSize = sizeof(WNDCLASSEX);
	//wcex.style = CS_HREDRAW | CS_VREDRAW;
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

	HACCEL hndAccel = LoadAccelerators(hInst, MAKEINTRESOURCE(MAINHKS));

	ShowWindow(hMainWind, nCmdShow);
	UpdateWindow(hMainWind);

	/*RAWINPUTDEVICE rid;
	rid.usUsagePage = 0x01;
	rid.usUsage = 0x02;
	rid.dwFlags = NULL;
	rid.hwndTarget = NULL;

	RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE));*/

	MSG msg = {};
	while(GetMessage(&msg, NULL, 0, 0))
	{
		/*if(pWhiteboard && GetForegroundWindow() == wbHandle)
		{
			if(inD3DRect)
			{
				HideWinCursor();
				SetCursorPos(pt.x, pt.y);
				DrawCursor(cursor, wbHandle, ptClient.x, ptClient.y);
			}
			else
				ShowWinCursor();
		}*/
		if(!TranslateAccelerator(hMainWind, hndAccel, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
//		else if(pWhiteboard && pWhiteboard->Initialized() && pWhiteboard->Interval())
//		{
//			pWhiteboard->BeginFrame();
//
//			/*if(inD3DRect && GetForegroundWindow() == wbHandle)
//			{
//				DrawCursor(cursor, wbHandle, ptClient.x, ptClient.y);
//				HideWinCursor();
//				SetCursorPos(pt.x, pt.y);
//
//				LIB_TCHAR textBuffer[128];
//				_stprintf(textBuffer, _T("In D3D Rect:%d\r\nMove Updates:%d\r\nLeft Down: %d\r\nLeftUp:%d"), inRect, nMoveUpdate, leftDown, leftUp);
//				SendMessage(textDisp, WM_SETTEXT, 0, (LPARAM)textBuffer);
//			}
//			else
//				ShowWinCursor();
//*/
//			pWhiteboard->Render();
//
//			pWhiteboard->EndFrame();
//
//			//EnterCriticalSection(&mouseSect);
//			pWhiteboard->SendMouseData(mServ, client);
//			//LeaveCriticalSection(&mouseSect);
//		}
	}

	return (int)msg.wParam;
}

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
			DialogBox(hInst, MAKEINTRESOURCE(VIEW_LOGS), hWnd, LogsProc);
			break;

		case ID_BUTTON_ENTER:
		case HK_ENTER:
		{
			if (GetWindowTextLength(textInput) == 0) break;

			if (!client->IsConnected())
			{
				MessageBox(hMainWind, _T("Must be connected to server"), _T("ERROR"), MB_ICONERROR);
				break;
			}
			const UINT len = SendMessage(textInput, WM_GETTEXTLENGTH, 0, 0) + 1;

			LIB_TCHAR* buffer = alloc<LIB_TCHAR>(len);
			SendMessage(textInput, WM_GETTEXT, len, (LPARAM)buffer);
			std::tstring str = buffer;
			dealloc(buffer);

			SendMessage(textInput, WM_SETTEXT, 0, (LPARAM)_T(""));
			EnableWindow(buttonEnter, false);

			std::tstring dest;
			Format::FormatText(str, dest, user, opts->TimeStamps());
			textBuffer.WriteLine(dest); 
			opts->WriteLine(dest);

			auto streamWriter = client->CreateOutStream(StreamWriter::SizeType(str), TYPE_DATA, MSG_DATA_TEXT);
			streamWriter.Write(str);
			client->SendServData(streamWriter);
			break;
		}

		case ID_SEND_FILES:
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

			LIB_TCHAR path[MAX_PATH]{};
			LIB_TCHAR dir[MAX_PATH]{};
			if(!FileMisc::BrowseFiles(path, hWnd, _T("All Files\0*.*\0"), FileMisc::OFN_DEFAULT | OFN_EXPLORER | OFN_ALLOWMULTISELECT, _T("Browse files")))
				break;

			_tcsncpy(dir, path, _tcslen(path) + 1);
			if (!PathIsDirectory(dir))
				PathRemoveFileSpec(dir);
			fileSend->SetFullPathSrc(std::tstring(dir));

			LIB_TCHAR* fileName = path + _tcslen(dir) + 1;
			while(_tcslen(fileName))
			{
				LIB_TCHAR *tmp = (LIB_TCHAR*)alloc<LIB_TCHAR>(_tcslen(dir) + 1 + _tcslen(fileName) + 1);
				PathCombine(tmp, dir, fileName);
				FileMisc::FileData data;
				File file(tmp, FILE_GENERIC_READ);
				data.dateModified = file.GetDate();
				data.size = file.GetSize();
				data.fileName = fileName;
				file.Close();
				fileSend->GetList().push_back(data);
				dealloc(tmp);
				fileName += _tcslen(fileName) + 1;
			}
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

			list = FileMisc::GetFileNameList(buffer);
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
		textBuffer.SetHandle(textDisp);

		textInput = CreateWindow(WC_EDIT, NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | WS_VSCROLL, 0, top, left, topLen, hWnd, (HMENU)ID_TEXTBOX_INPUT, hInst, NULL);
		listClients = CreateWindow(WC_LISTBOX, NULL, WS_CHILD | WS_VISIBLE | LBS_STANDARD | LBS_NOINTEGRALHEIGHT, left, 0, leftLen, top, hWnd, (HMENU)ID_LISTBOX_CLIENTS, hInst, NULL);

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
			CloseHandle(CreateThread(NULL, 0, WBPThread, construct<WBParams>(*(WBParams*)lParam), NULL, &wbPThreadID));
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

						AppendMenu(send, MF_STRING, ID_SEND_FILES, _T("Send File(s)"));
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
	{
		if(LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
		{
			//ShowWinCursor();
			SetFocus(textInput);
		}
		break;
	}

	case WM_CLOSE:
		if (pWhiteboard)
		{
			MessageBox(hMainWind, _T("Must exit whiteboard before closing client"), _T("Exit"), MB_OK);
			break;
		}

		DestroyWindow(hWnd);
		break;


	case WM_DESTROY:
		opts->SaveLog(); //log is removed if blank, and is blank if opts->SaveLogs() is false

		DestroyClient(client);
		CleanupNetworking();
		CoUninitialize();
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

LRESULT CALLBACK WbProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	//static HANDLE mouseThread;
	//static DWORD mouseThreadID;
	static USHORT prevX = 0, prevY = 0;
	static bool leftPressed = false;
	static USHORT width = 0, height = 0;
	Whiteboard& wb = *pWhiteboard;

	switch (message)
	{
	case WM_COMMAND:
	{
		switch(LOWORD(wParam))
		{
		case HK_BRUSH:
		{
			if(!wbCanDraw)
				break;

			const BYTE defColor = pWhiteboard->GetDefaultColor();
			BYTE clr;
			do
			{
				clr = rand() % 32;
			} while(clr == defColor);
			pWhiteboard->ChangeTool(Tool::PaintBrush, 0.0f, clr);
			break;
		}

		case HK_ERASER:
		{
			if(!wbCanDraw)
				break;

			pWhiteboard->ChangeTool(Tool::PaintBrush, 0.0f, pWhiteboard->GetDefaultColor());
			break;
		}

		case HK_BRUSHSIZE_DOWN:
		{
			if(!wbCanDraw)
				break;

			pWhiteboard->ChangeTool(Tool::PaintBrush, -1.0f, WBClientData::UNCHANGEDCOLOR);
			break;
		}

		case HK_BRUSHSIZE_UP:
		{
			if(!wbCanDraw)
				break;

			pWhiteboard->ChangeTool(Tool::PaintBrush, 1.0f, WBClientData::UNCHANGEDCOLOR);
			break;
		}
		}
		break;
	}

	case WM_MOUSEMOVE:
	{
		if(!wbCanDraw)
			break;

		if(leftPressed)
		{
			const USHORT x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
			if((x == prevX) && (y == prevY))
				break;

			if (x < width && y < height)
			{
				wb.GetMServ().OnMouseMove(x, y);
				prevX = x;
				prevY = y;
			}
		}
		break;
	}
	case WM_LBUTTONDOWN:
	{
		if(!wbCanDraw)
			break;

		SetCapture(hWnd);
		const USHORT x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);

		if (x < width && y < height)
		{
			wb.GetMServ().OnLeftPressed(x, y);
			leftPressed = true;

			prevX = x;
			prevY = y;
		}
		break;
	}

	case WM_LBUTTONUP:
	{
		if(!wbCanDraw)
			break;

		if(leftPressed)
		{
			const USHORT x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
			
			if (x < width && y < height)
			{
				wb.GetMServ().OnLeftReleased(x, y);
				leftPressed = false;
				prevX = x;
				prevY = y;
			}
			else
			{
				wb.GetMServ().OnLeftReleased(prevX, prevY);
			}
		}
		ReleaseCapture();
		break;
	}
	case WM_CREATE:
		wb.Initialize(hWnd);

		if(wbCanDraw)
			wb.StartThread(client);

		width = wb.GetD3D().GetWidth(), height = wb.GetD3D().GetHeight();

		client->SendMsg(TYPE_RESPONSE, MSG_RESPONSE_WHITEBOARD_INITED);
		break;

	case WM_ACTIVATE:
		if (pWhiteboard)
		{
			/*wb.GetD3D().BeginFrame();
			wb.GetD3D().EndFrame();*/
		}
		break;

	case WM_CLOSE:
		client->SendMsg(TYPE_WHITEBOARD, MSG_WHITEBOARD_LEFT);
		PostThreadMessage(wbPThreadID, WM_QUIT, NULL, NULL);
		WaitAndCloseHandle(wbPThread);

		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
	{
		EnableMenuItem(wbMenu, ID_WHITEBOARD_START, MF_ENABLED);
		EnableMenuItem(wbMenu, ID_WHITEBOARD_TERMINATE, MF_GRAYED);

		wbHandle = nullptr;
		UnregisterClass(wbClassName, hInst);
		destruct(pWhiteboard);
		break;
	}

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
			const UINT pos = str.find_last_of(_T(":"));
			Connect(str.substr(0, pos).c_str(), str.substr(pos + 1).c_str(), str[0] == _T('[') ? true : false, timeOut);
			if (client->IsConnected())
			{
				client->RecvServData();
				auto streamWriter = client->CreateOutStream(sizeof(float), TYPE_VERSION, MSG_VERSION_CHECK);
				streamWriter.Write(APPVERSION);

				client->SendServData(streamWriter);

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

INT_PTR CALLBACK LogsProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM)
{
	static HWND nameList;

	auto RefreshLogList = [hWnd]()
	{
		nameList = GetDlgItem(hWnd, LOGS_LB_NAME);
		SendMessage(nameList, LB_RESETCONTENT, 0, 0);
		auto& logs = opts->GetLogList();

		for(auto& it : logs)
		{
			TCHAR buffer[64];
			_stprintf(buffer, _T("%.12s - %d/%d/%d - %.2f"), it.fileName.c_str(), it.dateModified.wMonth, it.dateModified.wDay, it.dateModified.wYear, (float)it.size / 1024.0f);
			SendMessage(nameList, LB_ADDSTRING, 0, (LPARAM)buffer);
		}
	};

	switch(message)
	{
	case WM_COMMAND:
	{
		const short id = LOWORD(wParam);
		switch(id)
		{
		case IDOPEN:
		{
			const int index = SendMessage(nameList, LB_GETCURSEL, 0, 0);
			if(index != LB_ERR)
				DialogBoxParam(hInst, MAKEINTRESOURCE(ID_LOGREADER), hWnd, LogReaderProc, (LPARAM)index);
			break;
		}
		case IDREMOVE:
		{
			const int index = SendMessage(nameList, LB_GETCURSEL, 0, 0);
			if(index != LB_ERR)
				opts->RemoveLog(index);
			RefreshLogList();
			break;
		}
		case IDCLEARALL:
		{
			SendMessage(nameList, LB_RESETCONTENT, 0, 0);
			opts->ClearLogs();
			break;
		}

		case IDCANCEL:
		case IDCLOSE:
		{
			EndDialog(hWnd, id);
			break;
		}
		}
		break;
	}
	case WM_INITDIALOG:
	{
		RefreshLogList();

		return 1;
	}
	}
	return 0;
}

INT_PTR CALLBACK LogReaderProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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
		{
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
		text = GetDlgItem(hWnd, LOGREADER_TEXT);
		const int index = (int)lParam;
		DWORD nBytes;
		opts->ReadLog(index, nullptr, &nBytes);
		char* buffer = alloc<char>(nBytes);
		opts->ReadLog(index, buffer, &nBytes);
		SendMessage(text, WM_SETTEXT, NULL, (LPARAM)buffer);
		dealloc(buffer);
		return 1;
	}
	}
	return 0;
}

INT_PTR CALLBACK ManageProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM)
{
	static HWND list, ipv4Input, ipv6Input, portInput, add, remove;
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
			if ((SendMessage(ipv4Input, IPM_ISBLANK, 0, 0) == 0) && (SendMessage(portInput, WM_GETTEXTLENGTH, 0, 0) != 0))
				EnableWindow(add, TRUE);
			else
				EnableWindow(add, FALSE);
			break;
		}
		break;
		case ID_IP_ADD_IPV6:
		{
			switch (HIWORD(wParam))
			{
			case EN_CHANGE:
				if ((GetWindowTextLength(ipv6Input) != 0) && (SendMessage(portInput, WM_GETTEXTLENGTH, 0, 0) != 0))
					EnableWindow(add, TRUE);
				else
					EnableWindow(add, FALSE);
				break;
			}
			break;
		}
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
			const UINT ipv6Len = SendMessage(ipv6Input, WM_GETTEXTLENGTH, 0, 0);
			if (ipv6Len > 0)
			{
				LIB_TCHAR* temp = alloc<LIB_TCHAR>(ipv6Len + 1);
				SendMessage(ipv6Input, WM_GETTEXT, ipv6Len + 1, (LPARAM)temp);
				SendMessage(ipv6Input, WM_SETTEXT, NULL, (LPARAM)_T(""));
				LIB_TCHAR buffer[INET6_ADDRSTRLEN];
				_stprintf(buffer, _T("[%s]:%d"), temp, GetDlgItemInt(hWnd, ID_MANAGE_PORT, NULL, FALSE));
				dealloc(temp);
				servList.push_back(buffer);
				SendMessage(list, LB_ADDSTRING, 0, (LPARAM)buffer);
			}
			else
			{
				DWORD ip = 0;
				SendMessage(ipv4Input, IPM_GETADDRESS, 0, (LPARAM)&ip);
				LIB_TCHAR buffer[INET_ADDRSTRLEN] = {};
				_stprintf(buffer, _T("%d.%d.%d.%d:%d"), FIRST_IPADDRESS(ip), SECOND_IPADDRESS(ip), THIRD_IPADDRESS(ip), FOURTH_IPADDRESS(ip), GetDlgItemInt(hWnd, ID_MANAGE_PORT, NULL, FALSE));
				SendMessage(ipv4Input, IPM_CLEARADDRESS, 0, 0);
				servList.push_back(buffer);
				SendMessage(list, LB_ADDSTRING, 0, (LPARAM)buffer);
			}

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
		list = GetDlgItem(hWnd, ID_ADDR_LIST_MANAGE), ipv4Input = GetDlgItem(hWnd, ID_IP_ADD_IPV4), ipv6Input = GetDlgItem(hWnd, ID_IP_ADD_IPV6),
			portInput = GetDlgItem(hWnd, ID_MANAGE_PORT), add = GetDlgItem(hWnd, ID_BUTTON_ADD), remove = GetDlgItem(hWnd, ID_BUTTON_REMOVE);

		SendMessage(ipv6Input, EM_SETLIMITTEXT, INET6_ADDRSTRLEN, 0);
		
		SendMessage(portInput, EM_SETLIMITTEXT, 5, 0);
		SetDlgItemInt(hWnd, ID_MANAGE_PORT, DEFAULTPORT, FALSE);

		for (auto& i : servList)
			SendMessage(list, LB_ADDSTRING, 0, (LPARAM)i.c_str());

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

INT_PTR CALLBACK AuthenticateProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM)
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
				CloseHandle(CreateThread(NULL, 0, InvalidUserPass, hWnd, NULL, NULL));
				break;
			}

			const std::tstring send = user + _T(":") + pass;
			auto streamWriter = client->CreateOutStream(StreamWriter::SizeType(send), TYPE_REQUEST, MSG_REQUEST_AUTHENTICATION);
			streamWriter.Write(send);

			client->SendServData(streamWriter);

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
			CheckDlgButton(hWnd, ID_SAVELOGS, opts->SaveLogs() ? BST_CHECKED : BST_UNCHECKED);
			
			SendMessage(flashCount, WM_SETTEXT, 0, (LPARAM)To_String(opts->GetFlashCount()).c_str());
			EnableWindow(flashCount, opts->FlashTaskbar(hMainWind));

			SetWindowLongPtr(hWnd, DWL_MSGRESULT, PSNRET_NOERROR);
			break;
		}
		case PSN_APPLY:
		{
			LIB_TCHAR buffer[4];
			SendMessage(flashCount, WM_GETTEXT, 4, (LPARAM)buffer);

			bool prevSaveLogs = opts->SaveLogs();

			opts->SetGeneral(
				(BST_CHECKED == IsDlgButtonChecked(hWnd, ID_TIMESTAMPS)), 
				(BST_CHECKED == IsDlgButtonChecked(hWnd, ID_STARTUP)), 
				(BST_CHECKED == IsDlgButtonChecked(hWnd, ID_FLASH_TASKBAR)), 
				(BST_CHECKED == IsDlgButtonChecked(hWnd, ID_SAVELOGS)),
				GetDlgItemInt(hWnd, ID_FLASH_COUNT, NULL, FALSE));

			if (!prevSaveLogs && opts->SaveLogs())
			{
				opts->CreateLog();
				opts->WriteLine(textBuffer.GetText());
			}

			if (prevSaveLogs && !opts->SaveLogs())
				opts->RemoveTempLog();
			
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
				opts->SetDownloadPath(buffer);
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

INT_PTR CALLBACK WBSettingsProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM)
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
			auto streamWriter = client->CreateOutStream(sizeof(WBParams), TYPE_WHITEBOARD, MSG_WHITEBOARD_SETTINGS);
			streamWriter.Write(WBParams(
				(USHORT)GetDlgItemInt(hWnd, WHITEBOARD_RES_X, NULL, FALSE), 
				(USHORT)GetDlgItemInt(hWnd, WHITEBOARD_RES_Y, NULL, FALSE),
				(USHORT)GetDlgItemInt(hWnd, WHITEBOARD_FPS, NULL, FALSE), 
				(BYTE)ComboBox_GetCurSel(Colors)));

			client->SendServData(streamWriter);

			EnableMenuItem(wbMenu, ID_WHITEBOARD_START, MF_GRAYED);
			EnableMenuItem(wbMenu, ID_WHITEBOARD_TERMINATE, MF_ENABLED);

			wbCanDraw = true;

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

INT_PTR CALLBACK WBInviteProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM)
{
	static HWND draw, invite;
	static std::tstring usersel;
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

			auto streamWriter = client->CreateOutStream(StreamWriter::SizeType(usersel) + sizeof(bool), TYPE_REQUEST, MSG_REQUEST_WHITEBOARD);
			streamWriter.Write(BST_CHECKED == IsDlgButtonChecked(hWnd, ID_WHITEBOARD_CANDRAW));
			streamWriter.Write(usersel);

			client->SendServData(streamWriter);

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
		const UINT len = SendMessage(listClients, LB_GETTEXTLEN, i, 0) + 1;
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
