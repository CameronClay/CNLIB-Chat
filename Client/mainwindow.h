#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "clientinfo.h"
#include "manageserversdialog.h"
#include "optionsdialog.h"
#include "viewlogdialog.h"
#include "connectdialog.h"
#include "authenticatedialog.h"

#include <QMainWindow>
#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    //HWND CreateWBWindow(USHORT width, USHORT height);

    void ClearAll();

private slots:
    void on_actionConnect_triggered();

    void on_actionDisconnect_triggered();

    void on_actionManage_triggered();

    void on_actionOptions_triggered();

    void on_actionLogs_triggered();

    void on_actionAbout_triggered();

    void on_actionAbout_Qt_triggered();

    void on_btnEnter_clicked();

    void on_fontChanged(const QFont& font);

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

    void OnAdminKick();

private:
    void SetFont(const QFont& font);

    void FlashTaskbar();
    int FindClient(const QString& name) const;
    void AddClient(const QString& name);
    bool RemoveClient(const QString& name);
    void ClearClients();

    void InitDialogs();
    void InitHandlers();
    void InitMenus();

    bool eventFilter(QObject *object, QEvent *event);

private:
    Ui::MainWindow *ui;
    std::unique_ptr<OptionsDialog> optionsDialog;
    std::unique_ptr<ManageServersDialog> manageServersDialog;
    std::unique_ptr<ViewLogDialog> viewLogDialog;
    std::unique_ptr<ConnectDialog> connectDialog;
    std::unique_ptr<AuthenticateDialog> authenticateDialog;
    ClientInfo clientInfo;

    QMenu* cmenuAdmin;
    QAction* actMenuAdmin;
    QAction* actAdminKick;
};
#endif // MAINWINDOW_H
