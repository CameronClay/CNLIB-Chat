#include "resource.h"
#include "CNLIB\Typedefs.h"
#include "CNLIB\TCPServInterface.h"
#include "CNLIB\MsgStream.h"
#include "CNLIB\File.h"
#include "CNLIB\UPNP.h"
#include "Format.h"
#include "CNLIB\HeapAlloc.h"
#include "CNLIB\Messages.h"
#include "Whiteboard.h"
#include <assert.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "TCPCS.lib")


const float APPVERSION = .002f;


#define ID_TEXTBOX_DISPLAY 0

#define ID_MANAGE_ADMINS 1

struct Authent
{
	Authent(std::tstring user, std::tstring password)
		:
		user(user),
		password(password)
	{}

	std::tstring user, password;
};

const USHORT port = 565;

const UINT maxUserLen = 10;

const LIB_TCHAR folderName[] = _T("Cam's-Server");
const LIB_TCHAR servListFileName[] = _T("ServAuthentic.dat");
const LIB_TCHAR adminListFileName[] = _T("AdminList.dat");

LIB_TCHAR folderPath[MAX_PATH + 30];

CRITICAL_SECTION fileSect, authentSect;

HINSTANCE hInst;
HWND hMainWind, textDisp;
HMENU main, file;

static LIB_TCHAR szWindowClass[] = _T("Server");
static LIB_TCHAR szTitle[] = _T("Cameron's Server");

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK ManageAdminsProc(HWND, UINT, WPARAM, LPARAM);

USHORT screenWidth = 800, screenHeight = 600;

TCPServInterface* serv = nullptr;
Whiteboard* wb = nullptr;

std::vector<Authent> userList;
std::vector<std::tstring> adminList;

bool IsAdmin(std::tstring& user)
{
	for (auto& i : adminList)
	{
		if (i.compare(user) == 0)
		{
			return true;
		}
	}
	return false;
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

void AddToList(std::tstring& user, std::tstring& pass)
{
	EnterCriticalSection(&fileSect);

	File file(servListFileName, FILE_GENERIC_WRITE);
	file.SetCursor(FILE_END);
	file.WriteString(user);
	file.WriteString(pass);
	file.Close();

	LeaveCriticalSection(&fileSect);
}

void SendSingleUserData(TCPServInterface& serv, char* dat, DWORD nBytes, char type, char message)
{
	const UINT userLen = *(UINT*)&(dat[0]);
	//																	   - 1 for \0
	std::tstring user = std::tstring((LIB_TCHAR*)&(dat[sizeof(UINT)]), userLen - 1);
	ClientData* client = serv.FindClient(user);
	if(client == nullptr)
		return;

	const UINT offset = sizeof(UINT) + (userLen * sizeof(LIB_TCHAR));
	const DWORD nBy = (nBytes - offset) + MSG_OFFSET;
	char* msg = alloc<char>(nBy);

	msg[0] = type;
	msg[1] = message;
	memcpy(&msg[MSG_OFFSET], &dat[offset], nBytes - offset);

	HANDLE hnd = serv.SendClientData(msg, nBy, client->pc, true);
	WaitAndCloseHandle(hnd);
	dealloc(msg);

}

void TransferMessageWithName(TCPServInterface& serv, std::tstring& srcUserName, MsgStreamReader& streamReader)
{
	std::tstring user = streamReader.ReadEnd<LIB_TCHAR>();
	ClientData* client = serv.FindClient(user);
	if(client == nullptr)
		return;

	const DWORD nameLen = srcUserName.size() + 1;
	MsgStreamWriter streamWriter(streamReader.GetType(), streamReader.GetMsg(), nameLen * sizeof(LIB_TCHAR));

	streamWriter.Write(srcUserName.c_str(), nameLen);

	HANDLE hnd = serv.SendClientData(streamWriter, streamWriter.GetSize(), client->pc, true);
	WaitAndCloseHandle(hnd);
}

void TransferMessage(TCPServInterface& serv, MsgStreamReader& streamReader)
{
	std::tstring user = streamReader.ReadEnd<LIB_TCHAR>();
	ClientData* client = serv.FindClient(user);
	if(client == nullptr)
		return;

	MsgStreamWriter mStreamOut(streamReader.GetType(), streamReader.GetMsg(), 0);

	HANDLE hnd = serv.SendClientData(mStreamOut, mStreamOut.GetSize(), client->pc, true);
	WaitAndCloseHandle(hnd);
}

void RequestTransfer(TCPServInterface& serv, std::tstring& srcUserName, MsgStreamReader& streamReader)
{
	const UINT srcUserLen = streamReader.Read<UINT>();
	std::tstring user = streamReader.Read<LIB_TCHAR>(srcUserLen);
	ClientData* client = serv.FindClient(user);
	if(client == nullptr)
		return;

	const UINT nameLen = srcUserName.size() + 1;
	const DWORD nBy = sizeof(UINT) + (nameLen * sizeof(LIB_TCHAR)) + sizeof(double);
	MsgStreamWriter streamWriter(TYPE_REQUEST, MSG_REQUEST_TRANSFER, nBy);
	streamWriter.Write(nameLen);
	streamWriter.Write(srcUserName.c_str(), nameLen);
	streamWriter.Write<double>(streamReader.Read<double>());

	HANDLE hnd = serv.SendClientData(streamWriter, streamWriter.GetSize(), client->pc, true);
	WaitAndCloseHandle(hnd);
}

void DispIPMsg(Socket& pc, const LIB_TCHAR* str)
{
	if (pc.IsConnected())
	{
		LIB_TCHAR buffer[128] = {};
		pc.ToIp(buffer);
		_tcscat(buffer, str);
		DispText((BYTE*)buffer, _tcslen(buffer) * sizeof(LIB_TCHAR));
	}
}

void DisconnectHandler(ClientData* data)
{
	DispIPMsg(data->pc, _T(" has disconnected!"));
}

void ConnectHandler(ClientData* data)
{
	DispIPMsg(data->pc, _T(" has connected!"));
}

//Handles all incoming packets
void MsgHandler(void* server, void* client, BYTE* data, DWORD nBytes, void* obj)
{
	TCPServInterface& serv = *(TCPServInterface*)server;
	auto clients = serv.GetClients();
	const USHORT nClients = serv.ClientCount();
	ClientData* clint = (ClientData*)client;
	
	char* dat = (char*)(&data[MSG_OFFSET]);
	nBytes -= MSG_OFFSET;
	MsgStreamReader streamReader((char*)data, nBytes);
	const char type = streamReader.GetType(), msg = streamReader.GetMsg();

	switch (type)
	{
	case TYPE_REQUEST:
	{
		switch (msg)
		{
		case MSG_REQUEST_AUTHENTICATION:
		{
			EnterCriticalSection(&authentSect);

			std::tstring authent(streamReader.ReadEnd<LIB_TCHAR>());
			const UINT pos = authent.find(_T(":"));
			assert(pos != std::tstring::npos);

			std::tstring user = authent.substr(0, pos), pass = authent.substr(pos + 1, (nBytes / sizeof(LIB_TCHAR) - pos));
			bool inList = false, auth = false;

			//Compare credentials with those stored on the server
			for (auto it = userList.begin(), end = userList.end(); it != end; it++)
			{
				if (it->user.compare(user) == 0)
				{
					inList = true;
					if (it->password.compare(pass) == 0 && it->user.compare(clint->user) != 0)
					{
						auth = true;
					}
				}
			}

			ClientData* fClient = serv.FindClient(user);

			//If user is already connected, reject
			if(fClient && fClient->user.compare(user) == 0)
			{
				auth = false;
			}

			if (!inList)
			{
				//Add name to list
				userList.push_back(Authent(user, pass));
				auth = true;
				clint->user = user;
				AddToList(user, pass);
			}

			if (auth)
			{
				clint->user = user;

				//Confirm authentication
				serv.SendMsg(clint->pc, true, TYPE_RESPONSE, MSG_RESPONSE_AUTH_CONFIRMED);

				//Transfer currentlist of clients to new client
				for(USHORT i = 0; i < nClients; i++)
				{
					if(clients[i] != clint && !clients[i]->user.empty())
					{
						MsgStreamWriter streamWriter(TYPE_CHANGE, MSG_CHANGE_CONNECTINIT, (clients[i]->user.size() + 1) * sizeof(LIB_TCHAR));
						streamWriter.WriteEnd(clients[i]->user.c_str());

						HANDLE hnd = serv.SendClientData(streamWriter, streamWriter.GetSize(), clint->pc, true);
						WaitAndCloseHandle(hnd);
					}
				}

				MsgStreamWriter streamWriter(TYPE_CHANGE, MSG_CHANGE_CONNECT, (user.size() + 1) * sizeof(LIB_TCHAR));
				streamWriter.WriteEnd(user.c_str());

				HANDLE hnd = serv.SendClientData(streamWriter, streamWriter.GetSize(), clint->pc, false);
				WaitAndCloseHandle(hnd);
			}
			else
			{
				//Decline authentication
				serv.SendMsg(clint->pc, true, TYPE_RESPONSE, MSG_RESPONSE_AUTH_DECLINED);
			}

			LeaveCriticalSection(&authentSect);

			break;
		}

		case MSG_REQUEST_TRANSFER:
		{
			RequestTransfer(serv, clint->user, streamReader);
			break;
		}
		case MSG_REQUEST_WHITEBOARD:
		{
			if(IsAdmin(clint->user) || wb->IsCreator(clint->user))
				TransferMessageWithName(serv, clint->user, streamReader);
			else
				serv.SendMsg(clint->user, TYPE_ADMIN, MSG_ADMIN_NOT);
		}
		}
		break;
	}//TYPE_REQUEST
	case TYPE_DATA:
	{
		switch (msg)
		{
		case MSG_DATA_TEXT:
		{
			UINT nBy;
			LIB_TCHAR* msg = FormatMsg(TYPE_DATA, MSG_DATA_TEXT, (BYTE*)dat, nBytes, clint->user, nBy);

			HANDLE hnd = serv.SendClientData((char*)msg, nBy, clint->pc, false);
			WaitAndCloseHandle(hnd);
			dealloc(msg);
			break;
		}
		case MSG_DATA_MOUSE:
		{
			break;
		}
		break;
		}
	}//TYPE_DATA
	case TYPE_FILE:
	{
		switch (msg)
		{
		case MSG_FILE_LIST:
		{
			SendSingleUserData(serv, dat, nBytes, TYPE_FILE, MSG_FILE_LIST);
			break;
		}
		case MSG_FILE_DATA:
		{
			SendSingleUserData(serv, dat, nBytes, TYPE_FILE, MSG_FILE_DATA);
			break;
		}
		case MSG_FILE_SEND_CANCELED:
		case MSG_FILE_RECEIVE_CANCELED:
		{
			TransferMessage(serv, streamReader);
			break;
		}
		}
		break;
	}//TYPE_FILE
	case TYPE_RESPONSE:
	{
		switch (msg)
		{
		case MSG_RESPONSE_TRANSFER_DECLINED:
		case MSG_RESPONSE_TRANSFER_CONFIRMED:
		{
			TransferMessageWithName(serv, clint->user, streamReader);
			break;
		}
		case MSG_RESPONSE_WHITEBOARD_CONFIRMED:
		{
			wb->AddClient(clint->pc);
			const WBParams& wbParams = wb->GetParams();

			{
				MsgStreamWriter streamWriter(TYPE_WHITEBOARD, MSG_WHITEBOARD_ACTIVATE, sizeof(WBParams));
				streamWriter.Write<WBParams>(wbParams);
				HANDLE hnd = serv.SendClientData(streamWriter, streamWriter.GetSize(), clint->pc, true);
				WaitAndCloseHandle(hnd);
			}

			wb->SendBitmap(RectU(0, 0, wbParams.width, wbParams.height), clint->pc, true);

			{
				const DWORD nameLen = clint->user.size() + 1;
				MsgStreamWriter streamWriter(TYPE_RESPONSE, MSG_RESPONSE_WHITEBOARD_CONFIRMED, nameLen * sizeof(LIB_TCHAR));
				streamWriter.Write(clint->user.c_str(), nameLen);
				HANDLE hnd = serv.SendClientData(streamWriter, streamWriter.GetSize(), wb->GetPcs());
				WaitAndCloseHandle(hnd);
			}
		}
		case MSG_RESPONSE_WHITEBOARD_DECLINED:
		{
			
		}
		}
		break;
	}//TYPE_RESPONSE
	case TYPE_ADMIN:
	{
		switch (msg)
		{
		case MSG_ADMIN_KICK:
		{
			std::tstring user((LIB_TCHAR*)dat);
			if (IsAdmin(clint->user))
			{
				if (IsAdmin(user))//if the user to be kicked is not an admin
				{
					if (clint->user.compare(adminList.front()) != 0)//if the user who initiated the kick is not the super admin
					{
						serv.SendMsg(clint->user, TYPE_ADMIN, MSG_ADMIN_CANNOTKICK);
						break;
					}

					//Disconnect User
					TransferMessageWithName(serv, clint->user, streamReader);
					for(USHORT i = 0; i < nClients; i++)
					{
						if (clients[i]->user.compare(user) == 0)
						{
							DisconnectHandler(clients[i]);
							clients[i]->pc.Disconnect();
						}
					}

					break;
				}
				else
				{
					//Disconnect User
					TransferMessageWithName(serv, clint->user, streamReader);
					for(USHORT i = 0; i < nClients; i++)
					{
						if (clients[i]->user.compare(user) == 0)
						{
							DisconnectHandler(clients[i]);
							clients[i]->pc.Disconnect();
						}
					}
					break;
				}
			}

			serv.SendMsg(clint->user, TYPE_ADMIN, MSG_ADMIN_NOT);
			break;
		}
		}
		break;
	}//TYPE_ADMIN
	case TYPE_VERSION:
	{
		switch (msg)
		{
		case MSG_VERSION_CHECK:
		{
			if (*(float*)dat == APPVERSION)
				serv.SendMsg(clint->pc, true, TYPE_VERSION, MSG_VERSION_UPTODATE);
			else
				serv.SendMsg(clint->pc, true, TYPE_VERSION, MSG_VERSION_OUTOFDATE);

			break;
		}
		}
		break;
	}//TYPE_VERSION

	case TYPE_WHITEBOARD:
	{
		switch (msg)
		{
		case MSG_WHITEBOARD_SETTINGS:
		{
			if (!wb)
			{
				WBParams* params = (WBParams*)dat;

				MsgStreamWriter streamWriter(TYPE_WHITEBOARD, MSG_WHITEBOARD_ACTIVATE, sizeof(WBParams));
				streamWriter.Write(std::move(*params));
				HANDLE hnd = serv.SendClientData(streamWriter, streamWriter.GetSize(), clint->pc, true);
				WaitAndCloseHandle(hnd);

				wb = construct<Whiteboard>(Whiteboard(serv, *params, clint->user));

				wb->AddClient(clint->pc);
			}
			else
			{
				serv.SendMsg(clint->pc, true, TYPE_WHITEBOARD, MSG_WHITEBOARD_CANNOTCREATE);
			}
			break;
		}
		case MSG_WHITEBOARD_TERMINATE:
		{
			if(wb->IsCreator(clint->user))
			{
				serv.SendMsg(wb->GetPcs(), TYPE_WHITEBOARD, MSG_WHITEBOARD_TERMINATE);
				destruct(wb);
			}
			else
			{
				serv.SendMsg(clint->pc, true, TYPE_WHITEBOARD, MSG_WHITEBOARD_CANNOTTERMINATE);
			}

			break;
		}
		case MSG_WHITEBOARD_KICK:
		{
			std::tstring user((LIB_TCHAR*)dat);
			if(IsAdmin(clint->user) || wb->IsCreator(clint->user))
			{
				if(wb->IsCreator(user))//if the user to be kicked is the creator
				{
					serv.SendMsg(clint->user, TYPE_ADMIN, MSG_ADMIN_CANNOTKICK);
					break;
				}
				// BUG: Creator leaving before clients causes clients to crash
				// TODO: Before destroying whiteboard, remove all clients, check for wb before accessing.
				TransferMessageWithName(serv, clint->user, streamReader);
				break;
			}

			serv.SendMsg(clint->user, TYPE_ADMIN, MSG_ADMIN_NOT);
			break;
		}
		case MSG_WHITEBOARD_LEFT:
		{
			if (wb)
			{
				wb->RemoveClient( clint->pc );
				if (wb->IsCreator(clint->user))
				{
					serv.SendMsg(wb->GetPcs(), TYPE_WHITEBOARD, MSG_WHITEBOARD_TERMINATE);
					destruct(wb);
				}
			}
		break;
		}
		}
	}//TYPE_WHITEBOARD
	}
}

void SaveAdminList()
{
	File file(adminListFileName, FILE_GENERIC_WRITE, FILE_ATTRIBUTE_HIDDEN, CREATE_ALWAYS);

	for (auto& i : adminList)
		file.WriteString(i);

	file.Close();
}

void WinMainInit()
{
	FileMisc::GetFolderPath(CSIDL_APPDATA, folderPath);
	FileMisc::SetCurDirectory(folderPath);
	FileMisc::CreateFolder(folderName, FILE_ATTRIBUTE_HIDDEN);
	_stprintf(folderPath, _T("%s\\%s"), folderPath, folderName);
	FileMisc::SetCurDirectory(folderPath);

	File file(servListFileName, FILE_GENERIC_READ, FILE_ATTRIBUTE_HIDDEN);

	std::tstring user, pass;
	while (file.ReadString(user))
	{
		if (file.ReadString(pass))
		{
			userList.push_back(Authent(user, pass));
		}
		else break;
	}

	file.Close();


	file.Open(adminListFileName, FILE_GENERIC_READ, FILE_ATTRIBUTE_HIDDEN);

	while (file.ReadString(user))
	{
		adminList.push_back(user);
	}

	file.Close();

	InitializeCriticalSection(&fileSect);
	InitializeCriticalSection(&authentSect);

	InitializeNetworking();

	MapPort(port, _T("TCP"), _T("Cam's Serv"));

	serv = CreateServer(&MsgHandler, &ConnectHandler, &DisconnectHandler);

	LIB_TCHAR portA[6] = {};
	serv->AllowConnections(_itot(port, portA, 10));
}

void RecalcSizeVars(UINT width, UINT height)
{
	screenWidth = width;
	screenHeight = height;
}


int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{
	WinMainInit();

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
	wcex.lpszClassName = _T("Server");
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_APPLICATION));

	LIB_TCHAR title[30];
	_stprintf(title, _T("%s %.3f"), szTitle, APPVERSION);

	if (!RegisterClassEx(&wcex))
	{
		MessageBox(NULL, _T("Call to RegisterClassEx failed!"), title, NULL);
		return 1;
	}

	hInst = hInstance;

	hMainWind = CreateWindow(
		szWindowClass,
		title,
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
		MessageBox(NULL, _T("Call to CreateWindow failed!"), title, NULL);
		return 1;
	}

	ShowWindow(hMainWind, SW_SHOWDEFAULT);
	UpdateWindow(hMainWind);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case ID_MANAGE_ADMINS:
		{
			DialogBox(hInst, MAKEINTRESOURCE(MANAGE_ADMINS), hWnd, ManageAdminsProc);
			break;
		}
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	}
	case WM_CREATE:
	{
		textDisp = CreateWindow(WC_EDIT, NULL, WS_CHILD | WS_VISIBLE | ES_MULTILINE | WS_BORDER | ES_READONLY | WS_VSCROLL, 0, 0, screenWidth, screenHeight, hWnd, (HMENU)ID_TEXTBOX_DISPLAY, hInst, 0);

		main = CreateMenu();

		file = CreateMenu();
		AppendMenu(main, MF_POPUP, (UINT_PTR)file, _T("File"));
		AppendMenu(file, MF_STRING, ID_MANAGE_ADMINS, _T("Manage Admins"));

		SetMenu(hWnd, main);
		break;
	}

	case WM_SIZE:
	{
		RecalcSizeVars(LOWORD(lParam), HIWORD(lParam));
		MoveWindow(textDisp, 0, 0, screenWidth, screenHeight, TRUE);


		break;
	}


	case WM_ACTIVATE:
		//if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
			//SetFocus(textInput);
		break;

	case WM_DESTROY:
		DeleteCriticalSection(&fileSect);
		DeleteCriticalSection(&authentSect);
		DestroyServer(serv);
		CleanupNetworking();
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

INT_PTR CALLBACK ManageAdminsProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND list, userInput, add, remove;
	switch (message)
	{
	case WM_COMMAND:
	{
		const short id = LOWORD(wParam);
		switch (id)
		{
		case ID_TEXTBOX_ADMIN:
			switch (HIWORD(wParam))
			{
			case EN_CHANGE:
				if (GetWindowTextLength(userInput) != 0)
					EnableWindow(add, TRUE);
				else
					EnableWindow(add, FALSE);
				break;
			}
			break;
		case ID_ADMIN_LIST:
		{
			if (SendMessage(list, LB_GETCURSEL, 0, 0) != LB_ERR)
				EnableWindow(remove, TRUE);
			else
				EnableWindow(remove, FALSE);
			break;
		}
		case ID_ADMIN_REMOVE:
		{
			const UINT index = SendMessage(list, LB_GETCURSEL, 0, 0);
			const UINT len = SendMessage(list, LB_GETTEXTLEN, index, 0);
			LIB_TCHAR* buffer = alloc<LIB_TCHAR>(len + 1);
			SendMessage(list, LB_GETTEXT, index, (LPARAM)buffer);
			SendMessage(list, LB_DELETESTRING, index, 0);
			for (USHORT i = 0; i < adminList.size(); i++)
			{
				if (adminList[i].compare(buffer) == 0)
				{
					adminList.erase(adminList.begin() + i);
					break;
				}
			}
			dealloc(buffer);

			EnableWindow(remove, FALSE);
			break;
		}

		case ID_ADMIN_ADD:
		{
			const UINT len = SendMessage(userInput, WM_GETTEXTLENGTH, 0, 0) + 1;
			LIB_TCHAR* name = alloc<LIB_TCHAR>(len);
			SendMessage(userInput, WM_GETTEXT, len, (LPARAM)name);
			SendMessage(userInput, WM_SETTEXT, 0, (LPARAM)_T(""));

			adminList.push_back(name);
			SendMessage(list, LB_ADDSTRING, 0, (LPARAM)name);

			EnableWindow(add, FALSE);
			break;
		}


		case IDOK:
			SaveAdminList();
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
		list = GetDlgItem(hWnd, ID_ADDR_LIST_MANAGE), userInput = GetDlgItem(hWnd, ID_TEXTBOX_ADMIN), add = GetDlgItem(hWnd, ID_ADMIN_ADD), remove = GetDlgItem(hWnd, ID_ADMIN_REMOVE);
		SendMessage(userInput, EM_SETLIMITTEXT, maxUserLen, 0);
		for (auto& i : adminList)
			SendMessage(list, LB_ADDSTRING, 0, (LPARAM)i.c_str());

		EnableWindow(add, FALSE);
		EnableWindow(remove, FALSE);

		return 1;
	}
	}
	return 0;
}

