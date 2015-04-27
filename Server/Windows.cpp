#include "resource.h"
#include "TCPServ.h"
#include "File.h"
#include "WinFirewall.h"
#include "UPNP.h"
#include "Format.h"
#include "HeapAlloc.h"
#include "Messages.h"
#include "Whiteboard.h"
#include <assert.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#pragma comment(lib, "comctl32.lib")


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

const TCHAR folderName[] = _T("Cam's-Server");
const TCHAR servListFileName[] = _T("ServAuthentic.dat");
const TCHAR adminListFileName[] = _T("AdminList.dat");

TCHAR folderPath[MAX_PATH + 30];

CRITICAL_SECTION fileSect, authentSect;

HINSTANCE hInst;
HWND hMainWind, textDisp;
HMENU main, file;

static TCHAR szWindowClass[] = _T("Server");
static TCHAR szTitle[] = _T("Cameron's Server");

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK ManageAdminsProc(HWND, UINT, WPARAM, LPARAM);

USHORT screenWidth = 800, screenHeight = 600;

uqpc<TCPServ> serv;
Whiteboard* wb;

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
	TCHAR* buffer = (TCHAR*)alloc<char>(((len + 2) * sizeof(TCHAR)) + nBytes);
	SendMessage(textDisp, WM_GETTEXT, len, (LPARAM)buffer);
	if (len != 1) _tcscat(buffer, _T("\r\n"));
	_tcscat(buffer, (TCHAR*)data);
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

void SendSingleUserData(TCPServ& serv, BYTE* dat, DWORD nBytes, char type, char message)
{
	auto& clients = serv.GetClients();
	const UINT userLen = *(UINT*)&(dat[0]);
	//																	   - 1 for \0
	std::tstring user = std::tstring((TCHAR*)&(dat[sizeof(UINT)]), userLen - 1);
	for (UINT i = 0; i < clients.size(); i++)
	{
		if (clients[i].user.compare(user) == 0)
		{
			const UINT offset = sizeof(UINT) + (userLen * sizeof(TCHAR));
			const DWORD nBy = (nBytes - offset) + MSG_OFFSET;
			char* msg = alloc<char>(nBy);
			
			msg[0] = type;
			msg[1] = message;
			memcpy(&msg[MSG_OFFSET], &dat[offset], nBytes - offset);

			HANDLE hnd = serv.SendClientData(msg, nBy, clients[i].pc, true);
			TCPServ::WaitAndCloseHandle(hnd);
			dealloc(msg);
		}
	}
}

void TransferMessageWithName(TCPServ& serv, std::tstring& srcUserName, BYTE* dat)
{
	auto& clients = serv.GetClients();
	std::tstring user = std::tstring((TCHAR*)&(dat[MSG_OFFSET]));
	for (UINT i = 0; i < clients.size(); i++)
	{
		if (clients[i].user.compare(user) == 0)
		{
			const DWORD nBy = MSG_OFFSET + ((srcUserName.size() + 1) * sizeof(TCHAR));
			char* msg = alloc<char>(nBy);
			memcpy(msg, dat, MSG_OFFSET);
			memcpy(&msg[MSG_OFFSET], srcUserName.c_str(), nBy - MSG_OFFSET);

			HANDLE hnd = serv.SendClientData(msg, nBy, clients[i].pc, true);
			TCPServ::WaitAndCloseHandle(hnd);
			dealloc(msg);
		}
	}
}

void TransferMessage(TCPServ& serv, BYTE* dat)
{
	auto& clients = serv.GetClients();
	std::tstring user = std::tstring((TCHAR*)&(dat[MSG_OFFSET]));
	for (UINT i = 0; i < clients.size(); i++)
	{
		if (clients[i].user.compare(user) == 0)
		{
			const DWORD nBy = MSG_OFFSET;
			char* msg = alloc<char>(nBy);
			memcpy(msg, dat, MSG_OFFSET);

			HANDLE hnd = serv.SendClientData(msg, nBy, clients[i].pc, true);
			TCPServ::WaitAndCloseHandle(hnd);
			dealloc(msg);
		}
	}
}

void RequestTransfer(TCPServ& serv, std::tstring& srcUserName, BYTE* dat)
{
	auto& clients = serv.GetClients();
	const UINT srcUserLen = *(UINT*)&(dat[MSG_OFFSET]);
	std::tstring user = std::tstring((TCHAR*)&(dat[MSG_OFFSET + sizeof(UINT)]), srcUserLen - 1);
	
	for (UINT i = 0; i < clients.size(); i++)
	{
		if (clients[i].user.compare(user) == 0)
		{
			const UINT nameLen = srcUserName.size() + 1;
			const UINT nameSize = nameLen * sizeof(TCHAR);
			const DWORD nBy = MSG_OFFSET + sizeof(UINT) + nameSize + sizeof(double);
			DWORD pos = 0;
			char* msg = alloc<char>(nBy);

			memcpy(msg, dat, MSG_OFFSET);
			pos += MSG_OFFSET;

			*(UINT*)&(msg[pos]) = nameLen;
			pos += sizeof(UINT);

			memcpy(&msg[pos], srcUserName.c_str(), nameSize);
			pos += nameSize;

			*(double*)&(msg[pos]) = *(double*)&(dat[MSG_OFFSET + sizeof(UINT) + (srcUserLen * sizeof(TCHAR))]);
			pos += sizeof(double);

			HANDLE hnd = serv.SendClientData(msg, nBy, clients[i].pc, true);
			TCPServ::WaitAndCloseHandle(hnd);
			dealloc(msg);
		}
	}
}

void DispIPMsg(Socket& pc, const TCHAR* str)
{
	if (pc.IsConnected())
	{
		TCHAR buffer[128] = {};
		pc.ToIp(buffer);
		_tcscat(buffer, str);
		DispText((BYTE*)buffer, _tcslen(buffer) * sizeof(TCHAR));
	}
}

void DisconnectHandler(TCPServ::ClientData& data)
{
	DispIPMsg(data.pc, _T(" has disconnected!"));
}

void ConnectHandler(TCPServ::ClientData& data)
{
	DispIPMsg(data.pc, _T(" has connected!"));
}

//Handles all incoming packets
void MsgHandler(void* server, USHORT& index, BYTE* data, DWORD nBytes, void* obj)
{
	TCPServ& serv = *(TCPServ*)server;
	auto& clients = serv.GetClients();
	const char type = ((char*)data)[0], msg = ((char*)data)[1];
	BYTE* dat = (BYTE*)&(((char*)data)[MSG_OFFSET]);
	nBytes -= MSG_OFFSET;

	switch (type)
	{
	case TYPE_REQUEST:
	{
		switch (msg)
		{
		case MSG_REQUEST_AUTHENTICATION:
		{
			EnterCriticalSection(&authentSect);

			std::tstring authent = (TCHAR*)dat;
			const UINT pos = authent.find(_T(":"));
			std::tstring user = authent.substr(0, pos), pass = authent.substr(pos + 1, (nBytes / sizeof(TCHAR) - pos));
			bool inList = false, auth = false;

			//Compare credentials with those stored on the server
			for (auto it = userList.begin(), end = userList.end(); it != end; it++)
			{
				if (it->user.compare(user) == 0)
				{
					inList = true;
					if (it->password.compare(pass) == 0 && it->user.compare(clients[index].user) != 0)
					{
						auth = true;
					}
				}
			}

			//If user is already connected, reject
			for (UINT i = 0; i < clients.size(); i++)
			{
				if (clients[i].user.compare(user) == 0)
				{
					auth = false;
				}
			}

			if (!inList)
			{
				//Add name to list
				userList.push_back(Authent(user, pass));
				auth = true;
				clients[index].user = user;
				AddToList(user, pass);
			}

			if (auth)
			{
				clients[index].user = user;

				//Confirm authentication
				serv.SendMsg(clients[index].pc, true, TYPE_RESPONSE, MSG_RESPONSE_AUTH_CONFIRMED);

				//Transer currentlist of clients to new client
				for (UINT i = 0; i < clients.size(); i++)
				{
					if (i != index && !clients[i].user.empty())
					{
						const UINT nameLenS = clients[i].user.size() + 1;
						const UINT nBytesS = MSG_OFFSET + (sizeof(TCHAR) * (nameLenS));
						char* msgNameS = alloc<char>(nBytesS);//dealloced in send
						msgNameS[0] = TYPE_CHANGE;
						msgNameS[1] = MSG_CONNECTINIT;
						memcpy(&msgNameS[MSG_OFFSET], clients[i].user.c_str(), nameLenS * sizeof(TCHAR));

						HANDLE hnd = serv.SendClientData(msgNameS, nBytesS, clients[index].pc, true);
						TCPServ::WaitAndCloseHandle(hnd);
						dealloc(msgNameS);
					}
				}

				//Send new client to currently connected clients
				UINT nBy;
				std::tstring d = _T("has connected!");
				TCHAR* msgA = FormatMsg(TYPE_CHANGE, MSG_CONNECT, (BYTE*)d.c_str(), d.size() * sizeof(TCHAR), clients[index].user, nBy);

				HANDLE hnd = serv.SendClientData((char*)msgA, nBy, clients[index].pc, false);
				TCPServ::WaitAndCloseHandle(hnd);
				dealloc(msgA);
			}
			else
			{
				//Decline authentication
				serv.SendMsg(clients[index].pc, true, TYPE_RESPONSE, MSG_RESPONSE_AUTH_DECLINED);
			}

			LeaveCriticalSection(&authentSect);

			break;
		}

		case MSG_REQUEST_TRANSFER:
		{
			RequestTransfer(serv, clients[index].user, data);
			break;
		}
		case MSG_REQUEST_WHITEBOARD:
		{
			//need to allow creator to invite
			if(IsAdmin(clients[index].user) || wb->IsCreator(clients[index].user))
				TransferMessageWithName(serv, clients[index].user, data);
			else
				serv.SendMsg(clients[index].user, TYPE_ADMIN, MSG_ADMIN_NOT);
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
			TCHAR* msg = FormatMsg(TYPE_DATA, MSG_DATA_TEXT, dat, nBytes, clients[index].user, nBy);

			HANDLE hnd = serv.SendClientData((char*)msg, nBy, clients[index].pc, false);
			TCPServ::WaitAndCloseHandle(hnd);
			dealloc(msg);
			break;
		}
		case MSG_DATA_BITMAP:
		{
			// Not sure what server needs to do here, I'm guessing this is where
			// server would send the compressed images, whiteboard would call
			// serv->SendClientData??
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
			TransferMessage(serv, data);
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
			TransferMessageWithName(serv, clients[index].user, data);
			break;
		}
		case MSG_RESPONSE_WHITEBOARD_CONFIRMED:
		{
			wb->AddClient(clients[index].pc);

			const DWORD nBytes = MSG_OFFSET + sizeof(WBParams);
			char* msg = alloc<char>(nBytes);

			msg[0] = TYPE_WHITEBOARD;
			msg[1] = MSG_WHITEBOARD_ACTIVATE;
			memcpy(&msg[MSG_OFFSET], &wb->GetParams(), sizeof(WBParams));
			HANDLE hnd = serv.SendClientData(msg, nBytes, clients[index].pc, true);
			TCPServ::WaitAndCloseHandle(hnd);
			dealloc(msg);


			TransferMessageWithName(serv, clients[index].user, data);
		}
		case MSG_RESPONSE_WHITEBOARD_DECLINED:
		{
			// Possibly do nothing, still need to allow client to be able to 
			// join later
			TransferMessageWithName(serv, clients[index].user, data);
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
			std::tstring user = (TCHAR*)dat;
			if (IsAdmin(clients[index].user))
			{
				if (IsAdmin(user))//if the user to be kicked is not an admin
				{
					if (clients[index].user.compare(adminList.front()) != 0)//if the user who initiated the kick is not the super admin
					{
						serv.SendMsg(clients[index].user, TYPE_ADMIN, MSG_ADMIN_CANNOTKICK);
						break;
					}

					//Disconnect User
					TransferMessageWithName(serv, clients[index].user, data);
					for (USHORT i = 0; i < clients.size(); i++)
					{
						if (clients[i].user.compare(user) == 0)
						{
							DisconnectHandler(clients[i]);
							clients[i].pc.Disconnect();
						}
					}

					break;
				}
				else
				{
					//Disconnect User
					TransferMessageWithName(serv, clients[index].user, data);
					for (USHORT i = 0; i < clients.size(); i++)
					{
						if (clients[i].user.compare(user) == 0)
						{
							DisconnectHandler(clients[i]);
							clients[i].pc.Disconnect();
						}
					}
					break;
				}
			}

			serv.SendMsg(clients[index].user, TYPE_ADMIN, MSG_ADMIN_NOT);
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
				serv.SendMsg(clients[index].pc, true, TYPE_VERSION, MSG_VERSION_UPTODATE);
			else
				serv.SendMsg(clients[index].pc, true, TYPE_VERSION, MSG_VERSION_OUTOFDATE);

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


				const DWORD nBytes = MSG_OFFSET + sizeof(WBParams);
				char* msg = alloc<char>(nBytes);

				msg[0] = TYPE_WHITEBOARD;
				msg[1] = MSG_WHITEBOARD_ACTIVATE;
				memcpy(&msg[MSG_OFFSET], params, sizeof(WBParams));
				HANDLE hnd = serv.SendClientData(msg, nBytes, clients[index].pc, true);
				TCPServ::WaitAndCloseHandle(hnd);
				dealloc(msg);


				wb = construct<Whiteboard>(Whiteboard(serv, *params, clients[index].user));

				wb->AddClient(clients[index].pc);
			}
			else
			{
				serv.SendMsg(clients[index].pc, true, TYPE_WHITEBOARD, MSG_WHITEBOARD_CANNOTCREATE);
			}
			break;
		}
		case MSG_WHITEBOARD_TERMINATE:
		{
			if(wb->IsCreator(clients[index].user))
			{
				serv.SendMsg(wb->GetPcs(), TYPE_WHITEBOARD, MSG_WHITEBOARD_TERMINATE);
				destruct(wb);
			}
			else
			{
				serv.SendMsg(clients[index].pc, true, TYPE_WHITEBOARD, MSG_WHITEBOARD_CANNOTTERMINATE);
			}

			break;
		}
		case MSG_WHITEBOARD_KICK:
		{
			std::tstring user = (TCHAR*)dat;
			if(IsAdmin(clients[index].user) || wb->IsCreator(clients[index].user))
			{
				if(!wb->IsCreator(user))//if the user to be picked is not the creator
				{
					serv.SendMsg(clients[index].user, TYPE_ADMIN, MSG_ADMIN_CANNOTKICK);
					break;
				}
				TransferMessageWithName(serv, clients[index].user, data);
				break;
			}

			serv.SendMsg(clients[index].user, TYPE_ADMIN, MSG_ADMIN_NOT);
			break;
		}
		case MSG_WHITEBOARD_LEFT:
		{
			wb->RemoveClient(clients[index].pc);
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

//void main()
//{
//	InitializeNetworking();
//
//	FileMisc::GetFolderPath(FOLDERID_RoamingAppData, folderPath);
//	FileMisc::SetCurDirectory(folderPath);
//	FileMisc::CreateFolder(folderName, FILE_ATTRIBUTE_HIDDEN);
//	_stprintf(folderPath, _T("%s\\%s"), folderPath, folderName);
//	FileMisc::SetCurDirectory(folderPath);
//	_stprintf(authenListFilePath, _T("%s\\%s"), folderPath, servListFileName);
//	File file(authenListFilePath, FILE_GENERIC_READ);
//
//	std::tstring user, pass;
//	while (file.ReadString(user))
//	{
//		if (file.ReadString(pass))
//		{
//			userList.push_back(Authent(user, pass));
//		}
//		else break;
//	}
//	file.Close();
//
//	InitializeCriticalSection(&fileSect);
//	InitializeCriticalSection(&authentSect);
//	MapPort(port, _T("TCP"), _T("Test"));
//
//	TCHAR portA[5] = {};
//	TCPServ serv(_itot(port, portA, 10), 20, &MsgHandler, nullptr, 9);
//	serv.AllowConnections();
//
//	serv.WaitForRecvThread();
//
//
//	DeleteCriticalSection(&fileSect);
//	DeleteCriticalSection(&authentSect);
//	CleanupNetworking();
//}

void WinMainInit()
{
	InitializeNetworking();

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

	MapPort(port, _T("TCP"), _T("Cam's Serv"));

	TCHAR portA[5] = {};
	serv = uqpc<TCPServ>(construct<TCPServ>(TCPServ(20, &MsgHandler, nullptr, 9, &ConnectHandler, &DisconnectHandler)));

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

	TCHAR title[30];
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
			TCHAR* buffer = alloc<TCHAR>(len + 1);
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
			TCHAR* name = alloc<TCHAR>(len);
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

