#include "TCPClient.h"
#include "File.h"
#include "FileTransfer.h"

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdlib.h>
#include <string.h>
#include <list>
#include <assert.h>

#include "resource.h"
#include "Options.h"
#include "Format.h"
#include "HeapAlloc.h"
#include "UPNP.h"
#include "Messages.h"

#pragma comment(lib, "comctl32.lib")


const float APPVERSION = .002f;
const float CONFIGVERSION = .002f;


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

#define INACTIVE_TIME 15 * 1000
#define PING_FREQ 2.0f

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK ConnectProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK ManageProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK AuthenticateProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK Opt_GeneralProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK Opt_TextProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK Opt_FilesProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK RequestProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK TimerProc(HWND, UINT, WPARAM, LPARAM);


HINSTANCE hInst;
HWND hMainWind;

UINT screenWidth = 800, screenHeight = 600;
UINT left = 0, leftLen = 0;
UINT top = 0, topLen = 0;

uqpc<TCPClient> client;
uqpc<Options> opts;
uqpc<FileSend> fileSend;
uqpc<FileReceive> fileReceive;

static TCHAR szWindowClass[] = _T("Client");
static TCHAR szTitle[] = _T("Cameron's Client");

static TCHAR windowName[56];

const TCHAR folderName[] = _T("Cam's-Client");
const TCHAR servListFileName[] = _T("ServrecvList.txt");
TCHAR servListFilePath[MAX_PATH + 50];

const TCHAR optionsFileName[] = _T("Options.dat");
TCHAR optionsFilePath[MAX_PATH + 50];

const UINT maxUserLen = 10;

const UINT port = 566;
const float timeOut = 5.0f;

TCHAR folderPath[MAX_PATH + 30];

std::list<std::tstring> servList;

static HWND textDisp, textInput, listClients;
static HMENU main, file, options;

std::tstring user;

void RecalcSizeVars(UINT width, UINT height)
{
	screenWidth = width;
	screenHeight = height;
	left = screenWidth * (8.75f / 10.0f), leftLen = (screenWidth - left) - 10;
	top = screenHeight * (8.0f / 10.0f), topLen = screenHeight - top;
}

void DispText(BYTE* data, DWORD nBytes)
{
	const UINT len = SendMessage(textDisp, WM_GETTEXTLENGTH, 0, 0) + 1;
	TCHAR* buffer = (TCHAR*)alloc<char>(((len + 2) * sizeof(TCHAR)) + nBytes);
	SendMessage(textDisp, WM_GETTEXT, len, (LPARAM)buffer);
	if (len != 1) _tcscat(buffer, _T("\r\n"));
	_tcscat(buffer, (TCHAR*)data);
	SendMessage(textDisp, WM_SETTEXT, 0, (LPARAM)buffer);
	dealloc(buffer);
	SendMessage(textDisp, EM_LINESCROLL, 0, MAXLONG);
}

void Connect(const TCHAR* dest, const TCHAR* port, float timeOut)
{
	client->Connect(dest, port, timeOut);
	if (client->Connected())
	{
		EnableMenuItem(file, ID_SERV_CONNECT, MF_GRAYED);
		EnableMenuItem(file, ID_SERV_DISCONNECT, MF_ENABLED);
		DrawMenuBar(hMainWind);
	}
}

void ClearAll()
{
	SendMessage(textDisp, WM_SETTEXT, 0, (LPARAM)_T(""));
	SendMessage(textInput, WM_SETTEXT, 0, (LPARAM)_T(""));
	SendMessage(listClients, LB_RESETCONTENT, 0, (LPARAM)_T(""));
}

void Disconnect()
{
	ClearAll();
	EnableMenuItem(file, ID_SERV_CONNECT, MF_ENABLED);
	EnableMenuItem(file, ID_SERV_DISCONNECT, MF_GRAYED);
	DrawMenuBar(hMainWind);
	client->Disconnect();
}

void Flash()
{
	if (opts->FlashTaskbar())
	{
		FLASHWINFO info = { sizeof(FLASHWINFO), hMainWind, FLASHW_ALL | FLASHW_TIMERNOFG, opts->GetFlashCount() , 0 };
		FlashWindowEx(&info);
	}
}

//Handles all incoming packets
void MsgHandler(void* clientObj, BYTE* data, DWORD nBytes, void* obj)
{
	HRESULT res = CoInitialize(NULL);
	assert(SUCCEEDED(res));

	TCPClient& clint = *(TCPClient*)clientObj;
	const char type = ((char*)data)[0], msg = ((char*)data)[1];
	BYTE* dat = (BYTE*)&(((char*)data)[MSG_OFFSET]);
	nBytes -= MSG_OFFSET;

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
		case MSG_SERVERFULL:
		{
			Flash();
			MessageBox(hMainWind, _T("Server is full!"), _T("ERROR"), MB_ICONERROR);
			Disconnect();
			break;
		}
		case MSG_CONNECT:
		{
			UINT nBy = 0;
			TCHAR* buffer = FormatText(dat, nBytes, nBy, opts->TimeStamps());
			DispText((BYTE*)buffer, nBy);
			dealloc(buffer);
			std::tstring str = (TCHAR*)dat;
			const UINT first = str.find(_T("<"), 0) + 1, second = str.find(_T(">"), first), len = second - first;
			const std::tstring name = str.substr(first, len);

			if(SendMessage(listClients, LB_FINDSTRING, -1, (LPARAM)name.c_str()) == LB_ERR)
				SendMessage(listClients, LB_ADDSTRING, 0, (LPARAM)name.c_str());

			Flash();
			break;
		}
		case MSG_CONNECTINIT:
		{
			if(SendMessage(listClients, LB_FINDSTRING, -1, (LPARAM)dat) == LB_ERR)
				SendMessage(listClients, LB_ADDSTRING, 0, (LPARAM)dat);

			break;
		}
		case MSG_DISCONNECT:
		{
			std::tstring str = (TCHAR*)dat;
			const UINT first = str.find(_T("<"), 0) + 1, second = str.find(_T(">"), first), len = second - first;
			std::tstring name = str.substr(first, len);
			UINT item = 0;
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
					fileSend->RunCanceled();
				}
			}

			if((item = SendMessage(listClients, LB_FINDSTRING, -1, (LPARAM)name.c_str())) != LB_ERR)
				SendMessage(listClients, LB_DELETESTRING, item, 0);

			UINT nBy = 0;
			TCHAR* buffer = FormatText(dat, nBytes, nBy, opts->TimeStamps());
			DispText((BYTE*)buffer, nBy);
			dealloc(buffer);
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
			TCHAR* buffer = FormatText(dat, nBytes, nBy, opts->TimeStamps());
			DispText((BYTE*)buffer, nBy);
			dealloc(buffer);
			Flash();
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
			fileSend->StopSend();

			TCHAR buffer[255];
			_stprintf(buffer, _T("%s has declined your transfer request!"), dat);
			MessageBox(hMainWind, buffer, _T("DECLINED"), MB_ICONERROR);
			break;
		}

		case MSG_RESPONSE_TRANSFER_CONFIRMED:
		{
			TCHAR buffer[255];
			_stprintf(buffer, _T("%s has confirmed your transfer request!"), dat);
			MessageBox(hMainWind, buffer, _T("Success"), MB_OK);
			fileSend->StartSend();
			break;
		}
		}
		break;
	}//TYPE_RESPONSE
	case TYPE_FILE:
	{
		switch (msg)
		{
		case MSG_FILE_LIST:
		{
			fileReceive->RecvFileNameList(dat, nBytes, opts->GetDownloadPath());
			break;
		}
		case MSG_FILE_DATA:
		{
			fileReceive->RecvFile(dat, nBytes);
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
			fileSend->RunCanceled();
		}
		}
		break;
	}//TYPE_FILE
    case TYPE_REQUEST:
	{
		switch (msg)
		{
		case MSG_REQUEST_TRANSFER:
		{
			const UINT len = *(UINT*)dat;
			fileReceive->GetUser() = std::tstring((TCHAR*)&(dat[sizeof(UINT)]), len - 1);
			fileReceive->SetSize( *(long double*)&(dat[sizeof(UINT) + (len * sizeof(TCHAR))]));
			if (fileReceive->Running())
			{
				client->SendMsg(fileReceive->GetUser(), TYPE_RESPONSE, MSG_RESPONSE_TRANSFER_DECLINED);
				break;
			}

			Flash();
			TCHAR buffer[255];
			_stprintf(buffer, _T("%s wants to send you %g MB of file(s)"), fileReceive->GetUser().c_str(), fileReceive->GetSize());

			DialogBoxParam(hInst, MAKEINTRESOURCE(REQUEST), hMainWind, RequestProc, (LPARAM)buffer);
			break;
		}
		}
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
			TCHAR buffer[255];
			_stprintf(buffer, _T("%s has kicked you from the server!"), dat);
			MessageBox(hMainWind, buffer, _T("Kicked"), MB_ICONERROR);

			Disconnect();
			break;
		}
		case MSG_ADMIN_CANNOTKICK:
		{
			MessageBox(hMainWind, _T("You cannot kick an admin!"), _T("ERROR"), MB_ICONERROR);
			break;
		}
		}
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
		}
		break;
	}//TYPE_VERSION
	}
	CoUninitialize();
}

void DisconnectHandler() // for auto disconnection
{
	ClearAll();
	EnableMenuItem(file, ID_SERV_CONNECT, MF_ENABLED);
	EnableMenuItem(file, ID_SERV_DISCONNECT, MF_GRAYED);
	DrawMenuBar(hMainWind);

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

	client = uqpc<TCPClient>(construct<TCPClient>(TCPClient(&MsgHandler, &DisconnectHandler, NULL, 9)));
	fileSend = uqpc<FileSend>(construct<FileSend>(FileSend(*client.get(), hMainWind, &SendFinishedHandler, &SendCanceledHandler)));
	fileReceive = uqpc<FileReceive>(construct<FileReceive>(FileReceive(*client.get(), hMainWind, &ReceiveFinishedHandler, &ReceiveCanceledHandler)));
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

	ShowWindow(hMainWind, SW_SHOWDEFAULT);
	UpdateWindow(hMainWind);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(hMainWind, hndAccel, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
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

				break;

			case ID_BUTTON_ENTER:
			case ENTER:
			{
				if (GetWindowTextLength(textInput) == 0) break;

				if (!client->Connected())
				{
					MessageBox(hMainWind, _T("Must be connected to server"), _T("ERROR"), MB_ICONERROR);
					break;
				}
				const UINT len = SendMessage(textInput, WM_GETTEXTLENGTH, 0, 0) + 1;
				const UINT nBytes = (len * sizeof(TCHAR)) + MSG_OFFSET;
				char* msg = alloc<char>(nBytes);
				msg[0] = TYPE_DATA;
				msg[1] = MSG_DATA_TEXT;
				SendMessage(textInput, WM_GETTEXT, len, (LPARAM)&msg[MSG_OFFSET]);
				SendMessage(textInput, WM_SETTEXT, 0, (LPARAM)_T(""));
				EnableWindow(buttonEnter, false);

				UINT nBy;
				TCHAR* dispMsg = FormatText((BYTE*)&msg[MSG_OFFSET], nBytes - MSG_OFFSET, user, nBy, opts->TimeStamps());
				DispText((BYTE*)dispMsg, nBy);
				dealloc(dispMsg);

				HANDLE hnd = client->SendServData(msg, nBytes);
				TCPClient::WaitAndCloseHandle(hnd);
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

				TCHAR buffer[MAX_PATH];
				TCHAR path[MAX_PATH];
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

				TCHAR buffer[MAX_PATH];
				if (!FileMisc::BrowseFolder(_T("Browse directories"), buffer, hWnd))
					break;

				auto& list = fileSend->GetList();

				FileMisc::GetFileNameList(buffer, NULL, list);
				if(!list.empty())
				{
					std::tstring path = buffer;
					const UINT pos = path.find_last_of(_T("\\"));
					fileSend->SetFullPathSrc(path.substr(0, pos) + _T("\\"));
					std::tstring lastPath = path.substr(pos + 1) + _T("\\");
					for(auto& i : list)
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

				AppendMenu(send, MF_STRING | (fileSend->Running() ? MF_GRAYED : MF_ENABLED), ID_SEND_FILE, _T("Send File"));
				AppendMenu(send, MF_STRING | (fileSend->Running() ? MF_GRAYED : MF_ENABLED), ID_SEND_FOLDER, _T("Send Folder"));

				AppendMenu(admin, MF_STRING, ID_KICK, _T("Kick"));

				AppendMenu(hPop, MF_POPUP, (UINT_PTR)send, _T("Send"));
				AppendMenu(hPop, MF_POPUP, (UINT_PTR)admin, _T("Admin"));

				ClientToScreen(listClients, &pt);
				TrackPopupMenu(hPop, 0, pt.x, pt.y, 0, hWnd, NULL);
				DestroyMenu(hPop);
			}
		}
	}	break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	break;

	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
			SetFocus(textInput);
		break;

	case WM_DESTROY:
		CoUninitialize();
		CleanupNetworking();
		PostQuitMessage(0);
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
			TCHAR* buffer = alloc<TCHAR>(len + 1);
			SendMessage(list, LB_GETTEXT, index, (LPARAM)buffer);
			TCHAR temp[10] = {};
			Connect(buffer, _itot(port, temp, 10), timeOut);
			if (client->Connected())
			{
				client->RecvServData();

				const UINT nBytes = MSG_OFFSET + sizeof(float);
				char* msg = alloc<char>(nBytes);
				msg[0] = TYPE_VERSION;
				msg[1] = MSG_VERSION_CHECK;
				*(float*)(&msg[MSG_OFFSET]) = APPVERSION;

				HANDLE hnd = client->SendServData(msg, nBytes);
				TCPClient::WaitAndCloseHandle(hnd);
				dealloc(msg);

				EndDialog(hWnd, id);
			}
			else
			{
				MessageBox(hWnd, _T("Unable to connect to server"), _T("ERROR"), MB_ICONERROR);
			}
			dealloc(buffer);
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
	static HWND list, ipInput, add, remove;
	switch (message)
	{
	case WM_COMMAND:
	{
		const short id = LOWORD(wParam);
		switch (id)
		{
		case ID_IP_ADD:
			switch (HIWORD(wParam))
			{
			case EN_CHANGE:
				if (SendMessage(list, IPM_ISBLANK, 0, 0) == 0)
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
			TCHAR* buffer = alloc<TCHAR>(len + 1);
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
			TCHAR buffer[INET_ADDRSTRLEN] = {};
			_stprintf(buffer, _T("%d.%d.%d.%d"), FIRST_IPADDRESS(ip), SECOND_IPADDRESS(ip), THIRD_IPADDRESS(ip), FOURTH_IPADDRESS(ip));
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
		list = GetDlgItem(hWnd, ID_ADDR_LIST_MANAGE), ipInput = GetDlgItem(hWnd, ID_IP_ADD), add = GetDlgItem(hWnd, ID_BUTTON_ADD), remove = GetDlgItem(hWnd, ID_BUTTON_REMOVE);
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
			const UINT nBytes = (sizeof(TCHAR) * (send.size() + 1)) + MSG_OFFSET;
			char* buffer = alloc<char>(nBytes);
			buffer[0] = TYPE_REQUEST;
			buffer[1] = MSG_REQUEST_AUTHENTICATION;
			memcpy(&buffer[MSG_OFFSET], send.c_str(), (send.size() + 1) * sizeof(TCHAR));
			HANDLE hnd = client->SendServData(buffer, nBytes);
			TCPClient::WaitAndCloseHandle(hnd);
			dealloc(buffer);

			EndDialog(hWnd, id);
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
			CheckDlgButton(hWnd, ID_FLASH_TASKBAR, opts->FlashTaskbar() ? BST_CHECKED : BST_UNCHECKED);

			SendMessage(flashCount, WM_SETTEXT, 0, (LPARAM)To_String(opts->GetFlashCount()).c_str());
			EnableWindow(flashCount, opts->FlashTaskbar());

			SetWindowLongPtr(hWnd, DWL_MSGRESULT, PSNRET_NOERROR);
			break;
		}
		case PSN_APPLY:
		{
			TCHAR buffer[4];
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
			TCHAR buffer[MAX_PATH] = {};
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

INT_PTR CALLBACK RequestProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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
