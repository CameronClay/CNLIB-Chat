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

        case MSG_RESPONSE_WHITEBOARD_CONFIRMED:
        {
            const QString user = QString::fromStdWString(streamReader.Read<std::tstring>());
            emit MsgResponseWhiteboardConfirmed(user);
            break;
        }

        case MSG_RESPONSE_WHITEBOARD_DECLINED:
        {
            const QString user = QString::fromStdWString(streamReader.Read<std::tstring>());
            emit MsgResponseWhiteboardDeclined(user);
            break;
        }

        }// MSG_RESPONSES
        break;
    }//TYPE_RESPONSE

    case TYPE_REQUEST:
    {
        switch (msg)
        {
        case MSG_REQUEST_WHITEBOARD:
        {
            const WBInviteParams inviteParams = streamReader.Read<WBInviteParams>();
            const std::tstring name = streamReader.Read<std::tstring>();

            emit MsgRequestWhiteboard(QString::fromStdWString(name), inviteParams);
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

    case TYPE_WHITEBOARD:
    {
        switch(msg)
        {
        case MSG_WHITEBOARD_ACTIVATE:
        {
            const WhiteboardArgs wbargs = streamReader.Read<WhiteboardArgs>();
            emit MsgWhiteboardActivate(wbargs);

            break;
        }
        case MSG_WHITEBOARD_TERMINATE:
        {
            emit MsgWhiteboardTerminate();
            break;
        }
        case MSG_WHITEBOARD_CANNOTCREATE:
        {
            emit MsgWhiteboardCannotCreate();
            break;
        }
        case MSG_WHITEBOARD_CANNOTTERMINATE:
        {
            emit MsgWhiteboardCannotTerminate();
            break;
        }
        case MSG_WHITEBOARD_KICK:
        {
            const QString user = QString::fromStdWString(streamReader.Read<std::tstring>());
            emit MsgWhiteboardKickUser(user);
            break;
        }
        case MSG_WHITEBOARD_DRAWLINE:
        {
            const QPoint start = streamReader.Read<QPoint>();
            const QPoint end = streamReader.Read<QPoint>();
            const std::size_t penWidth = streamReader.Read<std::size_t>();
            const QColor penColor = QColor::fromRgb(streamReader.Read<QRgb>());
            emit MsgWhiteboardDrawLine(start, end, penWidth, penColor);
            break;
        }
        case MSG_WHITEBOARD_REQUEST_IMAGE:
        {
            emit MsgWhiteboardRequestImage();
            break;
        }
        case MSG_WHITEBOARD_FORWARD_IMAGE:
        {
            const auto fmt = streamReader.Read<QImage::Format>();
            const auto sz = streamReader.Read<std::size_t>();
            const auto* data = streamReader.Read<uchar>(sz);

            std::shared_ptr<uchar[]> dataOwned(new uchar[sz]()); //streamreader data becomes invalided after end of function call so this is needed
            memcpy(dataOwned.get(), data, sz * sizeof(uchar));

            const ImageInfo imgInfo(sz, fmt, dataOwned);
            emit MsgWhiteboardForwardImage(imgInfo);
            break;
        }
        case MSG_WHITEBOARD_CLEAR:
        {
            const QColor clr = streamReader.Read<QColor>();
            emit MsgWhiteboardClear(clr);
            break;
        }
        }
        break;
    }
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
