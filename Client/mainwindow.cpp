#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QTWidgets>
#include <cassert>

#include "StdAfx.h"
#include "CNLIB/TCPClientInterface.h"
#include "CNLIB/MsgStream.h"
#include "CNLIB/HeapAlloc.h"
#include "MessagesExt.h"
#include "Format.h"
#include "TextDisplay.h"
#include "Options.h"
//#include "Palette.h"
//#include "Whiteboard.h"
#include "DebugHelper.h"

//Built using QT

//HWND MainWindow::CreateWBWindow(USHORT width, USHORT height)
//{
//    clientInfo.pWhiteboard->Initialize(wbHandle);

//    if (clientInfo.wbCanDraw)
//        clientInfo.pWhiteboard->StartThread(client);

//    clientInfo.pWhiteboard->GetD3D().Present();
//    clientInfo.client->SendMsg(TYPE_RESPONSE, MSG_RESPONSE_WHITEBOARD_INITED);
//}


void MainWindow::ClearAll() {
    ClearClients();
    ui->txtEnter->clear();
}

void MainWindow::FlashTaskbar() {
    QApplication::alert(this, 1);
}

MainWindow::MainWindow(QWidget *parent)
    :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    clientInfo()
{
    ui->setupUi(this);

    InitDialogs();

    QStringListModel* listModel = new QStringListModel(nullptr);
    ui->listClients->setModel(listModel);

    ui->txtEnter->installEventFilter(this);

    InitMenus();
    InitHandlers();
}

void MainWindow::InitDialogs() {
    optionsDialog = std::make_unique<OptionsDialog>(this, clientInfo);
    viewLogDialog = std::make_unique<ViewLogDialog>(this, clientInfo);
    manageServersDialog = std::make_unique<ManageServersDialog>(this, clientInfo);
    connectDialog = std::make_unique<ConnectDialog>(this, clientInfo);
    authenticateDialog = std::make_unique<AuthenticateDialog>(this, clientInfo);
}

void MainWindow::InitHandlers() {
    connect(optionsDialog.get(), &OptionsDialog::OnFontChanged, this, &MainWindow::on_fontChanged);

    connect(&clientInfo, &ClientInfo::OnMsgChangeConnect, this, &MainWindow::OnMsgChangeConnect);
    connect(&clientInfo, &ClientInfo::OnMsgChangeConnectInit, this, &MainWindow::OnMsgChangeConnectInit);
    connect(&clientInfo, &ClientInfo::OnMsgChangeDisconnect, this, &MainWindow::OnMsgChangeDisconnect);

    connect(&clientInfo, &ClientInfo::OnMsgDataText, this, &MainWindow::OnMsgDataText);
    connect(&clientInfo, &ClientInfo::OnMsgResponseAuthDeclined, this, &MainWindow::OnMsgResponseAuthDeclined);
    connect(&clientInfo, &ClientInfo::OnMsgResponseAuthConfirmed, this, &MainWindow::OnMsgResponseAuthConfirmed);

    connect(&clientInfo, &ClientInfo::OnMsgAdminNot, this, &MainWindow::OnMsgAdminNot);
    connect(&clientInfo, &ClientInfo::OnMsgAdminKick, this, &MainWindow::OnMsgAdminKick);
    connect(&clientInfo, &ClientInfo::OnMsgAdminCannotKick, this, &MainWindow::OnMsgAdminCannotKick);

    connect(&clientInfo, &ClientInfo::OnMsgVersionUpToDate, this, &MainWindow::OnMsgVersionUpToDate);
    connect(&clientInfo, &ClientInfo::OnMsgVersionOutOfDate, this, &MainWindow::OnMsgVersionOutOfDate);

    connect(&clientInfo, &ClientInfo::OnClientConnect, this, &MainWindow::OnClientConnect);
    connect(&clientInfo, &ClientInfo::OnClientDisconnect, this, &MainWindow::OnClientDisconnect);

    connect(&clientInfo, &ClientInfo::OnFileSendFinished, this, &MainWindow::OnFileSendFinished);
    connect(&clientInfo, &ClientInfo::OnFileSendCanceled, this, &MainWindow::OnFileSendCanceled);
    connect(&clientInfo, &ClientInfo::OnFileReceivedFinished, this, &MainWindow::OnFileReceivedFinished);
    connect(&clientInfo, &ClientInfo::OnFileReceivedCanceled, this, &MainWindow::OnFileReceivedCanceled);

    connect(actAdminKick, SIGNAL(triggered()), this, SLOT(OnAdminKick()));
}

void MainWindow::InitMenus() {
    actMenuAdmin = new QAction(tr("&Admin"), ui->listClients);
    cmenuAdmin = new QMenu(tr("Admin"), ui->listClients);
    //&signifies shortcut, user can press K and tab to complete Kick
    actAdminKick = new QAction(tr("&Kick"), ui->listClients);
    cmenuAdmin->addAction(actAdminKick);
    actMenuAdmin->setMenu(cmenuAdmin);

    ui->listClients->addAction(actMenuAdmin);
}

void MainWindow::OnAdminKick() {
    QStringListModel* model = reinterpret_cast<QStringListModel*>(ui->listClients->model());
    const std::tstring userKick = model->data(ui->listClients->currentIndex()).toString().toStdWString();
    clientInfo.client->SendMsg(userKick, TYPE_ADMIN, MSG_ADMIN_KICK);
}

//returns index if found, INVALID_IDX if not found
int MainWindow::FindClient(const QString& name) const {
    int found = INVALID_IDX;
    if (clientInfo.client->ShuttingDown()) {
        return found;
    }

    QStringListModel* model = reinterpret_cast<QStringListModel*>(ui->listClients->model());
    int i = 0u;
    for(const auto& it : model->stringList()) {
        if(it.compare(name) == 0) {
            found = i;
            break;
        }
        ++i;
    }

    return found;
}
void MainWindow::AddClient(const QString& name) {
    QStringListModel* model = reinterpret_cast<QStringListModel*>(ui->listClients->model());
    const int row = model->rowCount();
    model->insertRows(row,1);
    model->setData(model->index(row, 0), QVariant(name));
}
bool MainWindow::RemoveClient(const QString& name) {
    QStringListModel* model = reinterpret_cast<QStringListModel*>(ui->listClients->model());
    int idx = FindClient(name);
    if(idx != INVALID_IDX) {
        model->removeRows(idx, 1);
        return true;
    }
    return false;
}
void MainWindow::ClearClients() {
    QStringListModel* model = reinterpret_cast<QStringListModel*>(ui->listClients->model());
    model->removeRows(0, model->rowCount());
}

void MainWindow::SetFont(const QFont& font) {
    ui->txtDisp->setFont(font);
    ui->txtEnter->setFont(font);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::on_fontChanged(const QFont& font) {
    SetFont(font);
}

void MainWindow::on_actionConnect_triggered() {
    connectDialog->setVisible(true);
}


void MainWindow::on_actionDisconnect_triggered() {
    clientInfo.client->Disconnect();
}


void MainWindow::on_actionManage_triggered() {
    manageServersDialog->setVisible(true);
}


void MainWindow::on_actionOptions_triggered() {
    optionsDialog->setVisible(true);
}


void MainWindow::on_actionLogs_triggered() {
    viewLogDialog->setVisible(true);
}


void MainWindow::on_actionAbout_triggered() {
    QMessageBox::about(this, tr("About Client"), tr("TCP chat client built using WNLIB and QT, similar to IRC. Whiteboard is currently unimplemented."));
}

void MainWindow::on_actionAbout_Qt_triggered() {
    QMessageBox::aboutQt(this, tr("About Qt"));
}

void MainWindow::on_btnEnter_clicked() {
    if(!clientInfo.client->IsConnected()) {
        QMessageBox::warning(this, tr("ERROR"), tr("Must be connected to a server!"));
        return;
    }
    const QString strq = ui->txtEnter->toPlainText();
    const std::tstring str = strq.toStdWString();

    std::tstring dest;
    Format::FormatText(str, dest, clientInfo.user, clientInfo.opts->TimeStamps());
    clientInfo.textBuffer.WriteLine(dest);
    clientInfo.opts->WriteLine(dest);

    auto streamWriter = clientInfo.client->CreateOutStream(StreamWriter::SizeType(str), TYPE_DATA, MSG_DATA_TEXT);
    streamWriter.Write(str);
    clientInfo.client->SendServData(streamWriter);
    ui->txtEnter->setText(tr(""));
    ui->txtDisp->setText(QString::fromStdWString(clientInfo.textBuffer.GetText()));
    //ui->txtDisp->setText(QString("%1\n%2").arg(ui->txtDisp->toPlainText(), strq));
}


void MainWindow::OnMsgChangeConnect(const QString& user) {
    AddClient(user);
}
void MainWindow::OnMsgChangeDisconnect(const QString& user, bool shuttingDown) {
    if(!shuttingDown) {
        RemoveClient(user);
        FlashTaskbar();
    }
}
void MainWindow::OnMsgChangeConnectInit(const QString& user) {
    AddClient(user);
}

void MainWindow::OnMsgDataText(const QString& str) {
    ui->txtDisp->setText(QString::fromStdWString(clientInfo.textBuffer.GetText()));

}
void MainWindow::OnMsgResponseAuthDeclined(const QString& user) {
    this->setEnabled(true);
    QMessageBox::warning(this, tr("Error"), tr("Invalid username/password or username is already taken"));
}
void MainWindow::OnMsgResponseAuthConfirmed(const QString& user) {
    AddClient(user);

    this->setEnabled(true);
    ui->btnEnter->setEnabled(true);
    QMessageBox::information(this, tr("Success"), tr("Success"));
}

void MainWindow::OnMsgAdminNot() {
    QMessageBox::information(this, tr("ERROR"), tr("You are not an admin"));
}
void MainWindow::OnMsgAdminKick(const QString& kickedBy) {
    QString msg = tr("%1 has kicked you from the server!").arg(kickedBy);
    QMessageBox::information(this, tr("Kicked"), msg);
    FlashTaskbar();
}
void MainWindow::OnMsgAdminCannotKick() {
    QMessageBox::warning(this, tr("ERROR"), tr("You cannot kick an admin!"));
}

void MainWindow::OnMsgVersionUpToDate() {
    authenticateDialog->setVisible(true);
}
void MainWindow::OnMsgVersionOutOfDate() {
    FlashTaskbar();
    QMessageBox::warning(this, tr("UPDATE!"), tr("Your client's version does not match the server's version"));
}

void MainWindow::OnClientDisconnect(bool unexpected) {
    ClearClients();

    ui->actionDisconnect->setEnabled(false);
    ui->actionConnect->setEnabled(true);
    ui->btnEnter->setEnabled(false);
    if(unexpected) {
        FlashTaskbar();
        QMessageBox::information(this, tr("ERROR"), tr("You have been disconnected from server!"));
    }
}
void MainWindow::OnClientConnect() {
    ui->actionDisconnect->setEnabled(true);
    ui->actionConnect->setEnabled(false);
}

void MainWindow::OnFileSendFinished(const QString& user) {
    QMessageBox::information(this, tr("Success"), tr("File transfer finished"));
}
void MainWindow::OnFileSendCanceled(const QString& user) {
    QMessageBox::warning(this, tr("ERROR"), QString("File transfer canceled by either you or %1").arg(user));
}
void MainWindow::OnFileReceivedFinished(const QString& user) {
    QMessageBox::information(this, tr("Success"), tr("File transfer finished"));
}
void MainWindow::OnFileReceivedCanceled(const QString& user) {
    QMessageBox::information(this, QString("File transfer canceled by either you or %1").arg(user), tr("File transfer finished"));
}

//returns true if should filter the event
bool MainWindow::eventFilter(QObject *object, QEvent *event) {
    if (object == ui->txtEnter && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return && !keyEvent->modifiers().testFlag(Qt::KeyboardModifiers::enum_type::ShiftModifier)) {
            ui->btnEnter->click();
            return true;
        }
    }

    return QMainWindow::eventFilter(object, event);
}
