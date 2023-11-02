#include "clientinfo.h"
#include "FuncUtils.h"
#include "MessagesExt.h"
#include "Format.h"
#include <memory>
#include <cassert>

ClientInfo::ClientInfo() {
    HRESULT res = CoInitialize(NULL);
    assert(SUCCEEDED(res));
    InitializeNetworking();

    const QString optionsDirPath = QString("%1/%2").arg(
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        Options::FOLDER_NAME
    );
    opts = uqpc<Options>(construct<Options>(optionsDirPath, QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)));
    opts->Load(ClientInfo::WINDOW_NAME);

    assert(InitDirectory());
    const QStringList servList = LoadServList();
    serverListModel = std::make_unique<DragStringListModel>(servList, nullptr);

    client = CreateClient(
        FuncUtils::WrapperHelper<0, TCPClientInterface&, MsgStreamReader>::get_wrapper(std::bind(&ClientInfo::MsgHandler, this, std::placeholders::_1, std::placeholders::_2)),
        FuncUtils::WrapperHelper<1, TCPClientInterface&, bool>::get_wrapper(std::bind(&ClientInfo::DisconnectHandler, this, std::placeholders::_1, std::placeholders::_2)),
        //FuncUtils::WrapperHelper<1, TCPClientInterface&, bool>::get_wrapper([this](TCPClientInterface& client, bool unexpected){this->DisconnectHandler(client, unexpected);}),
        5,
        BufferOptions(4096, 125000, 9, 1024),
        SocketOptions(0, 0, true)
    );
}

ClientInfo::~ClientInfo() {
    DestroyClient(client);
    opts->SaveLog(); //log is removed if blank, and is blank if opts->SaveLogs() is false
    CleanupNetworking();
    CoUninitialize();
}

bool ClientInfo::InitDirectory() {
    const QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString folderPath = QString("%1/%2").arg(appDataDir, Options::FOLDER_NAME);
    this->folderPath = folderPath;
    if(!QDir(folderPath).exists()) {
        if(!QDir().mkpath(folderPath)) {
            return false;
        }
    }
    return QDir::setCurrent(folderPath);
}

QStringList ClientInfo::LoadServList() {
    QStringList severList;
    QFile file(QString::fromWCharArray(Options::SERVLIST_FILENAME));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return severList;
    }
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    QString user;
    while(!in.atEnd()) {
        in >> user;
        severList.push_back(user);
    }

    file.close();
    return severList;
}

void ClientInfo::SaveServList(const QStringList& servList) {
    QFile file(QString::fromWCharArray(Options::SERVLIST_FILENAME));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    const std::size_t sz = servList.size();
    std::size_t i = 0u;
    for (const auto& it : servList) {
        out << it;
        if(i++ != sz - 1) {
            out << " ";
        }
    }

    file.close();
}

void ClientInfo::MsgHandler(TCPClientInterface&, MsgStreamReader streamReader) {
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

            emit OnMsgChangeConnect(QString::fromStdWString(name));
            break;
        }
        case MSG_CHANGE_CONNECTINIT:
        {
            const std::tstring name = streamReader.Read<std::tstring>();
            emit OnMsgChangeConnectInit(QString::fromStdWString(name));
            break;
        }
        case MSG_CHANGE_DISCONNECT:
        {
            const std::tstring name = streamReader.Read<std::tstring>();
            if (!client->ShuttingDown())
            {
                LIB_TCHAR buffer[128];
                _stprintf(buffer, _T("Server: %s has disconnected!"), name.c_str());

                std::tstring str;
                Format::FormatText(buffer, str, opts->TimeStamps());
                textBuffer.WriteLine(str);
                opts->WriteLine(str);
            }

            emit OnMsgChangeDisconnect(QString::fromStdWString(name), client->ShuttingDown());
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

            emit OnMsgDataText(QString::fromStdWString(str));
            break;
        }
        case MSG_DATA_BITMAP:
        {
//            if(pWhiteboard)
//            {
//                RectU rect = streamReader.Read<RectU>();
//                BYTE* pixels = streamReader.ReadEnd<BYTE>();
//                pWhiteboard->Frame(rect, pixels);
//            }
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
            const QString user = QString::fromStdWString(this->user);
            Disconnect();
            emit OnMsgResponseAuthDeclined(user);

            break;
        }

        case MSG_RESPONSE_AUTH_CONFIRMED:
        {
            const QString user = QString::fromStdWString(this->user);
            emit OnMsgResponseAuthConfirmed(user);
            break;
        }

//        case MSG_RESPONSE_WHITEBOARD_CONFIRMED:
//        {
//            LIB_TCHAR buffer[255];
//            _stprintf(buffer, _T("%s has joined the Whiteboard!"), streamReader.Read<std::tstring>().c_str());
//            QMessageBox::information(this, tr("Success"), QString::fromWCharArray(buffer));
//            break;
//        }

//        case MSG_RESPONSE_WHITEBOARD_DECLINED:
//        {
//            LIB_TCHAR buffer[255];
//            _stprintf(buffer, _T("%s has declined the whiteboard!"), streamReader.Read<std::tstring>().c_str());
//            QMessageBox::information(this, tr("DECLINED"), QString::fromWCharArray(buffer));
//            break;
//        }

        }// MSG_RESPONSES
        break;
    }//TYPE_RESPONSE

    case TYPE_REQUEST:
    {
        switch (msg)
        {
//        case MSG_REQUEST_WHITEBOARD:
//        {
//            wbCanDraw = streamReader.Read<bool>();
//            std::tstring name = streamReader.Read<std::tstring>();

//            LIB_TCHAR buffer[255];
//            _stprintf(buffer, _T("%s wants to display a whiteboard. Can Draw: %s"), name.c_str(), wbCanDraw ? _T("Yes") : _T("No"));

//            //DialogBoxParam(hInst, MAKEINTRESOURCE(REQUEST), hMainWind, RequestWBProc, (LPARAM)buffer);
//            break;
//        }
        }// MSG_REQUEST
        break;
    }//TYPE_REQUEST
    case TYPE_ADMIN:
    {
        switch (msg)
        {
        case MSG_ADMIN_NOT:
        {
            emit OnMsgAdminNot();
            break;
        }
        case MSG_ADMIN_KICK:
        {
            Disconnect();
            emit OnMsgAdminKick(QString::fromStdWString(streamReader.Read<std::tstring>()));
            break;
        }
        case MSG_ADMIN_CANNOTKICK:
        {
            emit OnMsgAdminCannotKick();
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
            emit OnMsgVersionUpToDate();
            break;
        }
        case MSG_VERSION_OUTOFDATE:
        {
            Disconnect();
            emit OnMsgVersionOutOfDate();
            break;
        }
        }// MSG_VERSION

        break;
    }//TYPE_VERSION

//    case TYPE_WHITEBOARD:
//    {
//        switch(msg)
//        {
//        case MSG_WHITEBOARD_ACTIVATE:
//        {
//            WBParams pParams = streamReader.Read<WBParams>();

//            pWhiteboard = construct<Whiteboard>(
//                *client,
//                palette,
//                pParams.width,
//                pParams.height,
//                pParams.fps,
//                pParams.clrIndex );

//           // SendMessage( hMainWind, WM_CREATEWIN, ID_WB, (LPARAM)&pParams );
//            break;
//        }
//        case MSG_WHITEBOARD_TERMINATE:
//        {
//            // in case user declined
//            //if(IsWindow(wbHandle))
//            //{
//                //SetForegroundWindow(wbHandle);
//                QMessageBox::warning(this, tr("Whiteboard Status"), tr("Whiteboard has been shutdown!"));
//                //SendMessage(wbHandle, WM_CLOSE, 0, 0);
//            //}
//            break;
//        }
//        case MSG_WHITEBOARD_CANNOTCREATE:
//        {
//            QMessageBox::warning(this, tr("ERROR"), tr("A whiteboard is already being displayed!"));
//            break;
//        }
//        case MSG_WHITEBOARD_CANNOTTERMINATE:
//        {
//            QMessageBox::warning(this, tr("ERROR"), tr("Only whiteboard creator can terminate the whiteboard!"));
//            break;
//        }
//        case MSG_WHITEBOARD_KICK:
//        {
//            LIB_TCHAR buffer[255];
//            _stprintf(buffer, _T("%s has removed you from the whiteboard!"), streamReader.Read<std::tstring>().c_str());
//            QMessageBox::warning(this, tr("Kicked"), QString::fromWCharArray(buffer));
//            //SendMessage(wbHandle, WM_CLOSE, 0, 0);
//            break;
//        }
//        }
//        break;
//    }
    }// TYPE
}

void ClientInfo::DisconnectHandler(TCPClientInterface& client, bool unexpected) { // for disconnection
    emit OnClientDisconnect(unexpected);
}

void ClientInfo::Connect(const LIB_TCHAR* dest, const LIB_TCHAR* port, bool ipv6, float timeOut) {
    if (client->Connect(dest, port, ipv6, timeOut))
    {
        //can only modify GUI from GUI thread, need to send signal instead
        emit OnClientConnect();
    }
}

void ClientInfo::Disconnect() {
    client->Disconnect();
}


void ClientInfo::SendFinishedHandler(const std::tstring& user) {
    emit OnFileSendFinished(QString::fromStdWString(user));
}

void ClientInfo::SendCanceledHandler(const std::tstring& user) {
    emit OnFileSendCanceled(QString::fromStdWString(user));
}

void ClientInfo::ReceiveFinishedHandler(const std::tstring& user) {
    emit OnFileReceivedFinished(QString::fromStdWString(user));
}

void ClientInfo::ReceiveCanceledHandler(const std::tstring& user) {
    emit OnFileReceivedCanceled(QString::fromStdWString(user));
}
