#ifndef SERVERINFO_H
#define SERVERINFO_H

#include <mutex>
#include <memory>
#include <QtWidgets>
#include "CNLIB/Typedefs.h"
#include "CNLIB/TCPServInterface.h"
#include "TextDisplay.h"
#include "whiteboard.h"

#include "DragStringListModel.h"

static constexpr int INVALID_IDX = -1;

class ServerInfo : public QObject
{
    Q_OBJECT
public:
    ServerInfo();
    ~ServerInfo();

    static constexpr float APPVERSION                = 1.0f;
    static constexpr float CONFIGVERSION             = .0025f;
    static constexpr USHORT DEFAULTPORT              = 565;
    static constexpr UINT USERNAME_LEN_MAX           = 10;

    static constexpr LIB_TCHAR FOLDER_NAME[]         = _T("Cam's-Server");
    static constexpr LIB_TCHAR SERV_LIST_FILENAME[]  = _T("ServAuthentic.dat");
    static constexpr LIB_TCHAR ADMIN_LIST_FILENAME[] = _T("AdminList.dat");
    static constexpr LIB_TCHAR OPTIONS_FILENAME[]    = _T("Options.dat");

    struct Authent
    {
        Authent(std::tstring user, std::tstring password)
            :
            user(user),
            password(password)
        {}

        std::tstring user, password;
    };

    USHORT port = DEFAULTPORT;

    std::mutex fileMut, authentMut;

    TCPServInterface* serv = nullptr;

    //Whiteboard* wb = nullptr;

    std::vector<Authent> userList;

    TextDisplay textBuffer;

    std::unique_ptr<DragStringListModel> adminsModel;

    void SaveAdminList(const QStringList& adminList);
    bool IsAdmin(const std::tstring& user) const;
    void AddToList(const std::tstring& user, const std::tstring& pass);

    QString GetSuperAdmin() const;

    void KickClient(const std::tstring& ititator, ClientData* client);

    void SendSingleUserData(TCPServInterface& serv, const char* dat, DWORD nBytes, char type, char message);
    void TransferMessageWithName(TCPServInterface& serv, const std::tstring& user, const std::tstring& srcUserName, MsgStreamReader& streamReader);
    void TransferMessage(TCPServInterface& serv, MsgStreamReader& streamReader);
    void RequestTransfer(TCPServInterface& serv, const std::tstring& srcUserName, MsgStreamReader& streamReader);

    void SaveOptions();

    void DispIPMsg(Socket& pc, const LIB_TCHAR* str);

private:
    QString folderPath;
    std::mutex wbMut;
    std::unique_ptr<Whiteboard> whiteboard;

    static QStringList LoadAdminList();
    bool InitDirectory();
    void ReadWriteConfig();

    bool CanConnect(TCPServInterface& serv, const Socket& socket);
    void DisconnectHandler(TCPServInterface& serv, ClientData* data, bool unexpected);
    void ConnectHandler(TCPServInterface& serv, ClientData* data);

    //Handles all incoming packets
    void MsgHandler(TCPServInterface& serv, ClientData* const clint, MsgStreamReader streamReader);

signals:
    void UpdateLog();
};

#endif // SERVERINFO_H
