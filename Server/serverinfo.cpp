#include "serverinfo.h"
#include "CNLIB/File.h"
#include "MessagesExt.h"
#include "Format.h"
#include "CNLIB/UPNP.h"
#include "FuncUtils.h"
#include <cassert>

ServerInfo::ServerInfo() {
    HRESULT res = CoInitialize(NULL);
    assert(SUCCEEDED(res));
    InitializeNetworking();

    //MapPort(port, _T("TCP"), _T("Cam's Serv"));

    assert(InitDirectory());

    const QStringList adminList = LoadAdminList();
    ReadWriteConfig();

    adminsModel = std::make_unique<DragStringListModel>(adminList, nullptr);

    serv = CreateServer(
        (sfunc)FuncUtils::WrapperHelper<0, TCPServInterface&, ClientData* const, MsgStreamReader>::get_wrapper(std::bind(&ServerInfo::MsgHandler, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)),
        (ConFunc)FuncUtils::WrapperHelper<1, TCPServInterface&, ClientData*>::get_wrapper(std::bind(&ServerInfo::ConnectHandler, this, std::placeholders::_1, std::placeholders::_2)),
        (DisconFunc)FuncUtils::WrapperHelper<2, TCPServInterface&, ClientData*, bool>::get_wrapper(std::bind(&ServerInfo::DisconnectHandler, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)),
        5,
        BufferOptions(4096, 125000, 9, 1024),
        SocketOptions(0, 0, true)
    );

    const std::tstring ports = std::to_wstring(port);
    serv->AllowConnections(
        ports.c_str(),
        FuncUtils::WrapperHelper<3, TCPServInterface&, const Socket&>::get_wrapper(std::bind(&ServerInfo::CanConnect, this, std::placeholders::_1, std::placeholders::_2))
    );

}

ServerInfo::~ServerInfo() {
    CleanupNetworking();
    CoUninitialize();
}

void ServerInfo::SendSingleUserData(TCPServInterface& serv, const char* dat, DWORD nBytes, char type, char message) {
    const UINT userLen = *(UINT*)dat;
    //																	   - 1 for \0
    std::tstring user = std::tstring((LIB_TCHAR*)(dat + sizeof(UINT)), userLen);
    ClientData* client = serv.FindClient(user);
    if(client == nullptr)
        return;

    const UINT offset = sizeof(UINT) + (userLen * sizeof(LIB_TCHAR));
    const DWORD nBy = (nBytes - offset) + MSG_OFFSET;
    BuffSendInfo msgInfo = serv.GetSendBuffer(nBy);
    char* msg = msgInfo.buffer;

    ((short*)msg)[0] = type;
    ((short*)msg)[1] = message;
    memcpy(msg + MSG_OFFSET, dat + offset, nBytes - offset);

    serv.SendClientData(msgInfo, nBy, client, true);
}

void ServerInfo::TransferMessageWithName(TCPServInterface& serv, const std::tstring& user, const std::tstring& srcUserName, MsgStreamReader& streamReader) {
    ClientData* client = serv.FindClient(user);
    if(client == nullptr)
        return;

    auto streamWriter = serv.CreateOutStream(StreamWriter::SizeType(srcUserName), streamReader.GetType(), streamReader.GetMsg());
    streamWriter.Write(srcUserName);

    serv.SendClientData(streamWriter, client, true);
}

void ServerInfo::TransferMessage(TCPServInterface& serv, MsgStreamReader& streamReader) {
    std::tstring user = streamReader.Read<std::tstring>();
    ClientData* client = serv.FindClient(user);
    if(client == nullptr)
        return;

    serv.SendMsg(client, true, streamReader.GetType(), streamReader.GetMsg());
}

void ServerInfo::RequestTransfer(TCPServInterface& serv, const std::tstring& srcUserName, MsgStreamReader& streamReader) {
    const UINT srcUserLen = streamReader.Read<UINT>();
    std::tstring user = streamReader.Read<LIB_TCHAR>(srcUserLen);
    ClientData* client = serv.FindClient(user);
    if(client == nullptr)
        return;

    MsgStreamWriter streamWriter = serv.CreateOutStream(TYPE_REQUEST, MSG_REQUEST_TRANSFER, StreamWriter::SizeType(srcUserName) + sizeof(double));
    streamWriter.Write(srcUserName);
    streamWriter.Write(streamReader.Read<double>());

    serv.SendClientData(streamWriter, client, true);
}

bool ServerInfo::InitDirectory() {
    const QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString folderPath = QString("%1/%2").arg(appDataDir, FOLDER_NAME);
    this->folderPath = folderPath;
    if(!QDir(folderPath).exists()) {
        if(!QDir().mkpath(folderPath)) {
            return false;
        }
    }
    return QDir::setCurrent(folderPath);
}

QString ServerInfo::GetSuperAdmin() const {
    return adminsModel->data(adminsModel->index(0, 0)).toString();
}

void ServerInfo::SaveAdminList(const QStringList& adminList) {
    QFile file(QString::fromWCharArray(ADMIN_LIST_FILENAME));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    const std::size_t sz = adminList.size();
    std::size_t i = 0u;
    for (const auto& it : adminList) {
        out << it;
        if(i++ != sz - 1) {
            out << " ";
        }
    }

    file.close();
}

QStringList ServerInfo::LoadAdminList() {
    QStringList adminList;
    QFile file(QString::fromWCharArray(ADMIN_LIST_FILENAME));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return adminList;
    }
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    QString user;
    while(!in.atEnd()) {
        in >> user;
        adminList.push_back(user);
    }

    file.close();
    return adminList;
}

void ServerInfo::SaveOptions() {
    QFile file(QString::fromWCharArray(OPTIONS_FILENAME));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }

    QDataStream out(&file);
    out.setByteOrder(QDataStream::ByteOrder::BigEndian);

    out << CONFIGVERSION << port;
    file.close();
}

void ServerInfo::ReadWriteConfig() {
    QFile file(QString::fromWCharArray(OPTIONS_FILENAME));
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QDataStream in(&file);
    in.setByteOrder(QDataStream::ByteOrder::BigEndian);

    float configVersion;
    in >> configVersion;
    if (configVersion == CONFIGVERSION) {
        in >> port;
    }
    else {
        port = DEFAULTPORT;
        SaveOptions();
    }

    file.close();
}

bool ServerInfo::IsAdmin(const std::tstring& user) const {
    for (const auto& i : adminsModel->stringList()) {
        if (i.compare(user) == 0) {
            return true;
        }
    }
    return false;
}

void ServerInfo::AddToList(const std::tstring& user, const std::tstring& pass) {
    std::lock_guard<std::mutex> lock_guard(fileMut);
    QFile file{QString::fromStdWString(SERV_LIST_FILENAME)};
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
        return;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    out << QString::fromStdWString(user) << QString::fromStdWString(pass);
    file.close();
}

void ServerInfo::DispIPMsg(Socket& pc, const LIB_TCHAR* str) {
    if (pc.IsConnected()) {
        textBuffer.WriteLine(pc.GetInfo().GetIp() + str);
        emit UpdateLog();
    }
}


bool ServerInfo::CanConnect(TCPServInterface& serv, const Socket& socket) {
    return true;
}

void ServerInfo::DisconnectHandler(TCPServInterface& serv, ClientData* data, bool unexpected) {
    auto streamWriter = serv.CreateOutStream(StreamWriter::SizeType(data->user), TYPE_CHANGE, MSG_CHANGE_DISCONNECT);
    streamWriter.Write(data->user);

    //nullptr because client is already disconnected at this point
    serv.SendClientData(streamWriter, nullptr, false);

    LIB_TCHAR buffer[128] = {};
    swprintf_s(buffer, _T("(%s) has disconnected!"), (!data->user.empty()) ? data->user.c_str() : _T("unknown"));
    DispIPMsg(data->pc, buffer);
}

void ServerInfo::ConnectHandler(TCPServInterface& serv, ClientData* data) {
    DispIPMsg(data->pc, _T(" has connected!"));
}

void ServerInfo::KickClient(const std::tstring& ititator, ClientData* client) {
    textBuffer.WriteLine(ititator + _T(" has kicked ") + client->user + _T(" from the server!"));
    emit UpdateLog();
    serv->DisconnectClient(client);
}


//Handles all incoming packets
void ServerInfo::MsgHandler(TCPServInterface& serv, ClientData* const clint, MsgStreamReader streamReader) {
    auto clients = serv.GetClients();
    const USHORT nClients = serv.ClientCount();

    char* dat = streamReader.GetData();
    const short type = streamReader.GetType(), msg = streamReader.GetMsg();

    switch (type)
    {
    case TYPE_REQUEST:
    {
        switch (msg)
        {
        case MSG_REQUEST_AUTHENTICATION:
        {
            std::lock_guard<std::mutex> lock_guard(authentMut);

            const std::tstring authent = streamReader.Read<std::tstring>();
            const std::size_t pos = authent.find(_T(":"));
            assert(pos != std::tstring::npos);

            const std::tstring user = authent.substr(0, pos), pass = authent.substr(pos + 1, authent.size() - pos);
            bool inList = false, auth = false;

            //Compare credentials with those stored on the server
            for (const auto& it : userList)
            {
                if (it.user.compare(user) == 0)
                {
                    inList = true;
                    if (it.password.compare(pass) == 0 && it.user.compare(clint->user) != 0)
                    {
                        auth = true;
                    }
                }
            }

            ClientData* fClient = serv.FindClient(user);

            //If user is already connected, reject
            if(fClient && fClient->user.compare(user) == 0) {
                auth = false;
            }
            else if(!inList) {
                auth = true;
            }

            if (auth)
            {
                clint->user = user;

                //Confirm authentication
                serv.SendMsg(clint, true, TYPE_RESPONSE, MSG_RESPONSE_AUTH_CONFIRMED);

                //Transfer currentlist of clients to new client
                for(USHORT i = 0; i < nClients; i++)
                {
                    if(clients[i] != clint && !clients[i]->user.empty())
                    {
                        {
                            auto streamWriter = serv.CreateOutStream(StreamWriter::SizeType(clients[i]->user), TYPE_CHANGE, MSG_CHANGE_CONNECTINIT);
                            streamWriter.Write(clients[i]->user);

                            //notify the client that authenticated which users are currently logged into the server
                            serv.SendClientData(streamWriter, clint, true);
                        }

                        {
                            auto streamWriter = serv.CreateOutStream(StreamWriter::SizeType(user), TYPE_CHANGE, MSG_CHANGE_CONNECT);
                            streamWriter.Write(user);

                            //notify all other clients this client has authenticated/logged in to server
                            serv.SendClientData(streamWriter, clients[i], true);
                        }

                    }
                }
            }
            else
            {
                //Decline authentication
                serv.SendMsg(clint, true, TYPE_RESPONSE, MSG_RESPONSE_AUTH_DECLINED);
            }

            if (!inList)
            {
                //Add name to list
                userList.push_back(ServerInfo::Authent(user, pass));
                auth = true;
                AddToList(user, pass);
            }

            break;
        }

        case MSG_REQUEST_TRANSFER:
        {
            RequestTransfer(serv, clint->user, streamReader);
            break;
        }
        case MSG_REQUEST_WHITEBOARD:
        {
            if(IsAdmin(clint->user) || whiteboard->IsCreator(QString::fromStdWString(clint->user)))
            {
                const bool canDraw = streamReader.Read<bool>();
                const std::tstring name = streamReader.Read<std::tstring>();
                auto fClint = serv.FindClient(name);
                if(!fClint) //ignore request if they are already participating in whiteboard
                    break;
                auto streamWriter = serv.CreateOutStream(StreamWriter::SizeType(canDraw, clint->user), streamReader.GetType(), streamReader.GetMsg());
                streamWriter.Write(canDraw);
                streamWriter.Write(clint->user);
                serv.SendClientData(streamWriter, fClint, true);
            }
            else {
                serv.SendMsg(clint->user, TYPE_ADMIN, MSG_ADMIN_NOT);
            }
            break;
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
            //<user>: text
            std::tstring str = streamReader.Read<std::tstring>();
            Format::FormatText(str, str, clint->user, false);

            auto streamWriter = serv.CreateOutStream(StreamWriter::SizeType(str), TYPE_DATA, MSG_DATA_TEXT);
            streamWriter.Write(str);

            serv.SendClientData(streamWriter, clint, false);
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
            SendSingleUserData(serv, dat, streamReader.GetDataSize(), TYPE_FILE, MSG_FILE_LIST);
            break;
        }
        case MSG_FILE_DATA:
        {
            SendSingleUserData(serv, dat, streamReader.GetDataSize(), TYPE_FILE, MSG_FILE_DATA);
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
            TransferMessageWithName(serv, streamReader.Read<std::tstring>(), clint->user, streamReader);
            break;
        }
        case MSG_RESPONSE_WHITEBOARD_CONFIRMED:
        {
            const WhiteboardArgs& wbArgs = whiteboard->GetArgs();

            //Send activate message to new client
            auto streamWriter = serv.CreateOutStream(sizeof(WhiteboardArgs), TYPE_WHITEBOARD, MSG_WHITEBOARD_ACTIVATE);
            streamWriter.Write(wbArgs);
            serv.SendClientData(streamWriter, clint, true);
            break;
        }
        case MSG_RESPONSE_WHITEBOARD_INITED:
        {
            if(!whiteboard->IsCreator(QString::fromStdWString(clint->user)))
            {
                {
                    //Tell current whiteboard members someone joined
                    auto streamWriter = serv.CreateOutStream(StreamWriter::SizeType(clint->user), TYPE_RESPONSE, MSG_RESPONSE_WHITEBOARD_CONFIRMED);
                    streamWriter.Write(clint->user);
                    serv.SendClientData(streamWriter, whiteboard->GetClients());
                }

                //Add client after so it doesnt send message to joiner
                whiteboard->AddClient(clint);

                {
                    //request image from whiteboard creator to send to new client
                    auto streamWriter = serv.CreateOutStream(0u, TYPE_WHITEBOARD, MSG_WHITEBOARD_REQUEST_IMAGE);
                    serv.SendClientData(streamWriter, whiteboard->GetCreatorClient(), true);
                }

            }
            else
            {
                whiteboard->AddClient(clint);
            }
            break;
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
            std::tstring user = streamReader.Read<std::tstring>();
            if (IsAdmin(clint->user))
            {
                if (IsAdmin(user))//if the user to be kicked is not an admin
                {
                    if (clint->user.compare(GetSuperAdmin().toStdWString()) != 0)//if the user who initiated the kick is not the super admin
                    {
                        serv.SendMsg(clint->user, TYPE_ADMIN, MSG_ADMIN_CANNOTKICK);
                        break;
                    }

                    //Disconnect User
                    TransferMessageWithName(serv, user, clint->user, streamReader);
                    for(USHORT i = 0; i < nClients; i++)
                    {
                        if (clients[i]->user.compare(user) == 0)
                        {
                            KickClient(user, clients[i]);
                        }
                    }

                    break;
                }
                else
                {
                    //Disconnect User
                    TransferMessageWithName(serv, user, clint->user, streamReader);
                    for(USHORT i = 0; i < nClients; i++)
                    {
                        if (clients[i]->user.compare(user) == 0)
                        {
                            KickClient(user, clients[i]);
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
            if (streamReader.Read<float>() == ServerInfo::APPVERSION) {
                serv.SendMsg(clint, true, TYPE_VERSION, MSG_VERSION_UPTODATE);
            }
            else {
                serv.SendMsg(clint, true, TYPE_VERSION, MSG_VERSION_OUTOFDATE);
            }

            break;
        }
        }
        break;
    }//TYPE_VERSION

    case TYPE_WHITEBOARD:
    {
        switch (msg)
        {
        //sent from client when whiteboard is started
        case MSG_WHITEBOARD_SETTINGS:
        {
            if (!whiteboard)
            {
                const WhiteboardArgs params = streamReader.Read<WhiteboardArgs>();

                auto streamWriter = serv.CreateOutStream(sizeof(WhiteboardArgs), TYPE_WHITEBOARD, MSG_WHITEBOARD_ACTIVATE);
                streamWriter.Write(params);
                serv.SendClientData(streamWriter, clint, true);

                std::lock_guard<std::mutex> lock_guard(wbMut);
                whiteboard = std::make_unique<Whiteboard>(QString::fromStdWString(clint->user), clint, params);
            }
            else
            {
                serv.SendMsg(clint, true, TYPE_WHITEBOARD, MSG_WHITEBOARD_CANNOTCREATE);
            }
            break;
        }
        case MSG_WHITEBOARD_TERMINATE:
        {
            if(whiteboard->IsCreator(QString::fromStdWString(clint->user)))
            {
                const auto clients = whiteboard->GetClients();
                if(clients.empty())
                {
                    serv.SendMsg(clients, TYPE_WHITEBOARD, MSG_WHITEBOARD_TERMINATE);
                }
                std::lock_guard<std::mutex> lock_guard(wbMut);
                whiteboard.reset();
            }
            else
            {
                serv.SendMsg(clint, true, TYPE_WHITEBOARD, MSG_WHITEBOARD_CANNOTTERMINATE);
            }

            break;
        }
        case MSG_WHITEBOARD_KICK:
        {
            const std::tstring user = streamReader.Read<std::tstring>();
            if(IsAdmin(clint->user) || whiteboard->IsCreator(QString::fromStdWString(clint->user)))
            {
                if(whiteboard->IsCreator(QString::fromStdWString(user)))//if the user to be kicked is the creator
                {
                    serv.SendMsg(clint->user, TYPE_ADMIN, MSG_ADMIN_CANNOTKICK);
                    break;
                }

                TransferMessageWithName(serv, user, clint->user, streamReader);
                break;
            }

            serv.SendMsg(clint->user, TYPE_ADMIN, MSG_ADMIN_NOT);
            break;
        }
        case MSG_WHITEBOARD_LEFT:
        {
            if (whiteboard)
            {
                whiteboard->RemoveClient(clint);
                if (whiteboard->IsCreator(QString::fromStdWString(clint->user)))
                {
                    const auto clients = whiteboard->GetClients();
                    if(!clients.empty())
                    {
                        serv.SendMsg(clients, TYPE_WHITEBOARD, MSG_WHITEBOARD_TERMINATE);
                    }

                    std::lock_guard<std::mutex> lock_guard(wbMut);
                    whiteboard.reset();
                }
            }
            break;
        }
        case MSG_WHITEBOARD_FORWARD_IMAGE:
        case MSG_WHITEBOARD_DRAWLINE:
        case MSG_WHITEBOARD_CLEAR: {
            if (whiteboard)
            {
                MsgStreamWriter streamWriter = serv.CreateOutStream(streamReader.GetDataSize(), streamReader.GetType(), streamReader.GetMsg());
                streamWriter.Write(streamReader.GetData(), streamReader.GetDataSize());

                const std::vector<ClientData*> clients = whiteboard->GetClients();
                for(const auto& client : clients) {
                    if(client->pc != clint->pc) {
                        serv.SendClientData(streamWriter, client, true);
                    }
                }
            }
            break;
        }
        }
    }//TYPE_WHITEBOARD
    }
}
