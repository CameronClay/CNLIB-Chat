#ifndef CLIENTINFO_H
#define CLIENTINFO_H

#include <QTWidgets>
#include "CNLIB/TCPClientInterface.h"
#include "CNLIB/HeapAlloc.h"
#include "TextDisplay.h"
#include "Options.h"
//#include "Whiteboard.h"
#include <memory>
#include <cstddef>

#include "DragStringListModel.h"

static constexpr int INVALID_IDX = -1;

class ClientInfo : public QObject {
    Q_OBJECT
public:
    ClientInfo();
    ~ClientInfo();

    static constexpr USHORT DEFAULTPORT            = 565;
    static constexpr std::size_t USERNAME_LEN_MAX  = 10;

    static constexpr LIB_TCHAR WINDOW_NAME[]       = _T("Cam's Client v1.00");

    TCPClientInterface* client;
    uqpc<Options> opts;

    USHORT port = DEFAULTPORT;
    float timeOut = 5.0f;

    std::unique_ptr<DragStringListModel> serverListModel;

    //// Whiteboard declarations
    //Whiteboard *pWhiteboard = nullptr;
    //bool wbCanDraw = false;
    //// Whiteboard declarations end

    HANDLE wbPThread;
    DWORD wbPThreadID;

    TextDisplay textBuffer;
    std::tstring user;

    void SaveServList(const QStringList& servList);

    void Connect(const LIB_TCHAR* dest, const LIB_TCHAR* port, bool ipv6, float timeOut);
    void Disconnect();

private:
    QString folderPath;

    QStringList LoadServList();
    bool InitDirectory();

    void MsgHandler(TCPClientInterface&, MsgStreamReader streamReader);
    void DisconnectHandler(TCPClientInterface& client, bool unexpected);
    void SendFinishedHandler(const std::tstring& user);
    void SendCanceledHandler(const std::tstring& user);
    void ReceiveFinishedHandler(const std::tstring& user);
    void ReceiveCanceledHandler(const std::tstring& user);

signals:
    void OnMsgChangeConnect(const QString& user);
    void OnMsgChangeDisconnect(const QString& user, bool shuttingDown);
    void OnMsgChangeConnectInit(const QString& user);

    void OnMsgDataText(const QString& str);
    void OnMsgResponseAuthDeclined(const QString& user);
    void OnMsgResponseAuthConfirmed(const QString& user);

    void OnMsgAdminNot();
    void OnMsgAdminKick(const QString& kickedBy);
    void OnMsgAdminCannotKick();

    void OnMsgVersionUpToDate();
    void OnMsgVersionOutOfDate();

    void OnClientDisconnect(bool unexpected);
    void OnClientConnect();

    void OnFileSendFinished(const QString& user);
    void OnFileSendCanceled(const QString& user);
    void OnFileReceivedFinished(const QString& user);
    void OnFileReceivedCanceled(const QString& user);
};

#endif // CLIENTINFO_H
