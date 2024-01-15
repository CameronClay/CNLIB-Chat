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

//Built using QT

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
    ui->listClients->setEditTriggers(QAbstractItemView::NoEditTriggers);

    ui->txtEnter->installEventFilter(this);

    InitMenus();
    InitHandlers();
}

void MainWindow::InitDialogs() {
    optionsDialog = std::make_unique<OptionsDialog>(this, clientInfo);
    viewLogDialog = std::make_unique<ViewLogDialog>(this, clientInfo);

    manageServersDialog = std::make_unique<ManageServersDialog>(this, clientInfo);
    connectDialog       = std::make_unique<ConnectDialog>(this, clientInfo);
    authenticateDialog  = std::make_unique<AuthenticateDialog>(this, clientInfo);

    whiteboardDialog       = std::make_unique<Whiteboard>(this, clientInfo);
    startWhiteboardDialog  = std::make_unique<StartWhiteboardDialog>(this, clientInfo);
    inviteWhiteboardDialog = std::make_unique<InviteWhiteboardDialog>(this, clientInfo);
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

    connect(&clientInfo, &ClientInfo::MsgResponseWhiteboardConfirmed, this, &MainWindow::OnMsgResponseWhiteboardConfirmed);
    connect(&clientInfo, &ClientInfo::MsgResponseWhiteboardDeclined, this, &MainWindow::OnMsgResponseWhiteboardDeclined);
    connect(&clientInfo, &ClientInfo::MsgRequestWhiteboard, this, &MainWindow::OnMsgRequestWhiteboard);
    connect(&clientInfo, &ClientInfo::MsgWhiteboardActivate, this, &MainWindow::OnMsgWhiteboardActivate);
    connect(&clientInfo, &ClientInfo::MsgWhiteboardTerminate, this, &MainWindow::OnMsgWhiteboardTerminate);
    connect(&clientInfo, &ClientInfo::MsgWhiteboardCannotCreate, this, &MainWindow::OnMsgWhiteboardCannotCreate);
    connect(&clientInfo, &ClientInfo::MsgWhiteboardCannotTerminate, this, &MainWindow::OnMsgWhiteboardCannotTerminate);
    connect(&clientInfo, &ClientInfo::MsgWhiteboardKickUser, this, &MainWindow::OnMsgWhiteboardKickUser);
    connect(&clientInfo, &ClientInfo::MsgWhiteboardDrawLine, this, &MainWindow::OnMsgWhiteboardDrawLine);
    connect(&clientInfo, &ClientInfo::MsgWhiteboardRequestImage, this, &MainWindow::OnMsgWhiteboardRequestImage);
    connect(&clientInfo, &ClientInfo::MsgWhiteboardForwardImage, this, &MainWindow::OnMsgWhiteboardForwardImage);
    connect(&clientInfo, &ClientInfo::MsgWhiteboardClear, this, &MainWindow::OnMsgWhiteboardClear);

    connect(&clientInfo, &ClientInfo::OnClientConnect, this, &MainWindow::OnClientConnect);
    connect(&clientInfo, &ClientInfo::OnClientDisconnect, this, &MainWindow::OnClientDisconnect);

    connect(&clientInfo, &ClientInfo::OnFileSendFinished, this, &MainWindow::OnFileSendFinished);
    connect(&clientInfo, &ClientInfo::OnFileSendCanceled, this, &MainWindow::OnFileSendCanceled);
    connect(&clientInfo, &ClientInfo::OnFileReceivedFinished, this, &MainWindow::OnFileReceivedFinished);
    connect(&clientInfo, &ClientInfo::OnFileReceivedCanceled, this, &MainWindow::OnFileReceivedCanceled);

    connect(whiteboardDialog.get(), &Whiteboard::ClientClosedWhiteboard, this, &MainWindow::OnClientClosedWhiteboard);

    connect(actAdminKick, SIGNAL(triggered()), this, SLOT(OnMenuAdminKick()));

    connect(actWhiteboardInvite, SIGNAL(triggered()), this, SLOT(OnMenuWhiteboardInvite()));
    connect(actWhiteboardKick, SIGNAL(triggered()), this, SLOT(OnMenuWhiteboardKick()));

    connect(startWhiteboardDialog.get(), &StartWhiteboardDialog::StartWhiteboard, this, &MainWindow::OnWhiteboardStart);
    connect(inviteWhiteboardDialog.get(), &InviteWhiteboardDialog::InviteWhiteboard, this, &MainWindow::OnInviteWhiteboard);
}

void MainWindow::InitMenus() {
    actMenuAdmin = new QAction(tr("&Admin"), ui->listClients);
    cmenuAdmin = new QMenu(tr("Admin"), ui->listClients);
    //&signifies shortcut, user can press K and tab to complete Kick
    actAdminKick = new QAction(tr("&Kick"), ui->listClients);
    cmenuAdmin->addAction(actAdminKick);
    actMenuAdmin->setMenu(cmenuAdmin);

    ui->listClients->addAction(actMenuAdmin);

    actMenuWhiteboard = new QAction(tr("&Whiteboard"), ui->listClients);
    cmenuWhiteboard = new QMenu(tr("Whiteboard"), ui->listClients);
    //&signifies shortcut, user can press K and tab to complete Kick
    actWhiteboardInvite = new QAction(tr("&Invite"), ui->listClients);
    cmenuWhiteboard->addAction(actWhiteboardInvite);
    actWhiteboardKick = new QAction(tr("&Kick"), ui->listClients);
    cmenuWhiteboard->addAction(actWhiteboardKick);
    actMenuWhiteboard->setMenu(cmenuWhiteboard);

    actMenuWhiteboard->setEnabled(false);

    ui->listClients->addAction(actMenuAdmin);
    ui->listClients->addAction(actMenuWhiteboard);
}

void MainWindow::OnMenuAdminKick() {
    QStringListModel* model = reinterpret_cast<QStringListModel*>(ui->listClients->model());
    const std::tstring userKick = model->data(ui->listClients->currentIndex()).toString().toStdWString();
    clientInfo.client->SendMsg(userKick, TYPE_ADMIN, MSG_ADMIN_KICK);
}

void MainWindow::OnMenuWhiteboardInvite() {
    if(whiteboardDialog->isVisible()) {
        inviteWhiteboardDialog->setVisible(true);
    }
}

void MainWindow::OnMenuWhiteboardKick() {
    if(whiteboardDialog->isVisible()) {
        QStringListModel* model = reinterpret_cast<QStringListModel*>(ui->listClients->model());
        const std::tstring userKick = model->data(ui->listClients->currentIndex()).toString().toStdWString();
        clientInfo.client->SendMsg(userKick, TYPE_WHITEBOARD, MSG_WHITEBOARD_KICK);
    }
}

void MainWindow::OnWhiteboardStart(const WhiteboardArgs& args) {
    whiteboardDialog->Init(args);
    whiteboardDialog->SetInviteParams(WBInviteParams{true});

    auto streamWriter = clientInfo.client->CreateOutStream(sizeof(WhiteboardArgs), TYPE_WHITEBOARD, MSG_WHITEBOARD_SETTINGS);
    streamWriter.Write(args);

    clientInfo.client->SendServData(streamWriter);
}

void MainWindow::OnInviteWhiteboard(const WBInviteParams& params) {
    QStringListModel* model = reinterpret_cast<QStringListModel*>(ui->listClients->model());
    const std::tstring usersel = model->data(ui->listClients->currentIndex()).toString().toStdWString();
    auto streamWriter = clientInfo.client->CreateOutStream(StreamWriter::SizeType(usersel) + sizeof(bool), TYPE_REQUEST, MSG_REQUEST_WHITEBOARD);
    streamWriter.Write(params.canDraw);
    streamWriter.Write(usersel);

    clientInfo.client->SendServData(streamWriter);
}

void MainWindow::on_actionStart_triggered()
{
    startWhiteboardDialog->setVisible(true);
    whiteboardDialog->setVisible(false);
}


void MainWindow::on_actionTerminate_triggered()
{
    startWhiteboardDialog->setVisible(false);
    whiteboardDialog->setVisible(false);
    ui->actionStart->setEnabled(true);
    ui->actionTerminate->setEnabled(false);
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
    const int idx = FindClient(name);
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
    QMessageBox::about(this, tr("About Client"), tr("TCP chat client built using WNLIB and QT, similar to IRC."));
}

void MainWindow::on_actionAbout_Qt_triggered() {
    QMessageBox::aboutQt(this, tr("About Qt"));
}

void MainWindow::on_btnEnter_clicked() {
    if(!clientInfo.client->IsConnected()) {
        QMessageBox::warning(this, tr("Error"), tr("Must be connected to a server!"));
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
}


void MainWindow::OnMsgChangeConnect(const QString& user) {
    AddClient(user);
    ui->txtDisp->setText(QString::fromStdWString(clientInfo.textBuffer.GetText()));
}
void MainWindow::OnMsgChangeDisconnect(const QString& user, bool shuttingDown) {
    if(!shuttingDown) {
        RemoveClient(user);
        FlashTaskbar();
    }
}
void MainWindow::OnMsgChangeConnectInit(const QString& user) {
    AddClient(user);
    ui->txtDisp->setText(QString::fromStdWString(clientInfo.textBuffer.GetText()));
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
    ui->menuWhiteboard->setEnabled(true);
    QMessageBox::information(this, tr("Success"), tr("Success"));
}

void MainWindow::OnMsgAdminNot() {
    QMessageBox::information(this, tr("Error"), tr("You are not an admin"));
}
void MainWindow::OnMsgAdminKick(const QString& kickedBy) {
    QString msg = tr("%1 has kicked you from the server!").arg(kickedBy);
    QMessageBox::information(this, tr("Kicked"), msg);
    FlashTaskbar();
}
void MainWindow::OnMsgAdminCannotKick() {
    QMessageBox::warning(this, tr("Error"), tr("You cannot kick an admin!"));
}

void MainWindow::OnMsgVersionUpToDate() {
    authenticateDialog->setVisible(true);
}
void MainWindow::OnMsgVersionOutOfDate() {
    FlashTaskbar();
    QMessageBox::warning(this, tr("Update!"), tr("Your client's version does not match the server's version"));
}

void MainWindow::OnMsgResponseWhiteboardDeclined(const QString& user) {
    QMessageBox::information(this, tr("Declined"), QString("%1 has declined to join the whiteboard!").arg(user));
}
void MainWindow::OnMsgResponseWhiteboardConfirmed(const QString& user) {
    QMessageBox::information(this, tr("Confirmed"), QString("%1 has joined the Whiteboard!").arg(user));
}
void MainWindow::OnMsgRequestWhiteboard(const QString& user, const WBInviteParams& inviteParams) {
    if(whiteboardDialog->isVisible()) {
        clientInfo.client->SendMsg(TYPE_RESPONSE, MSG_RESPONSE_WHITEBOARD_DECLINED);
        return;
    }

    FlashTaskbar();

    const QMessageBox::StandardButton btn = QMessageBox::question(this, tr("Whiteboard request"), QString("%1 wants to display a whiteboard. Can Draw: %2").arg(user, inviteParams.canDraw ? tr("Yes") : tr("No")));
    if(btn == QMessageBox::StandardButton::Yes) {
        whiteboardDialog->SetInviteParams(inviteParams);
        clientInfo.client->SendMsg(TYPE_RESPONSE, MSG_RESPONSE_WHITEBOARD_CONFIRMED);
    }
    else if(btn == QMessageBox::StandardButton::No) {
        clientInfo.client->SendMsg(TYPE_RESPONSE, MSG_RESPONSE_WHITEBOARD_DECLINED);
    }
}
void MainWindow::OnMsgWhiteboardActivate(const WhiteboardArgs& wbargs) {
    if(!whiteboardDialog->isVisible()) {
        whiteboardDialog->Init(wbargs);
        whiteboardDialog->setVisible(true);
        actMenuWhiteboard->setEnabled(true);
        ui->actionStart->setEnabled(false);
        ui->actionTerminate->setEnabled(true);

        clientInfo.client->SendMsg(TYPE_RESPONSE, MSG_RESPONSE_WHITEBOARD_INITED);
    }
}
void MainWindow::WhiteboardShutdownHelper() {
    whiteboardDialog->setVisible(false);
    ui->menuWhiteboard->setEnabled(true);
    ui->actionStart->setEnabled(true);
    ui->actionTerminate->setEnabled(false);
}
void MainWindow::OnMsgWhiteboardTerminate() {
    if(whiteboardDialog->isVisible()) {
        QMessageBox::warning(this, tr("Whiteboard Status"), tr("Whiteboard has been shutdown!"));
        WhiteboardShutdownHelper();
    }
}
void MainWindow::OnMsgWhiteboardCannotCreate() {
    QMessageBox::warning(this, tr("Error"), tr("A whiteboard is already being displayed!"));
}
void MainWindow::OnMsgWhiteboardCannotTerminate() {
    QMessageBox::warning(this, tr("Error"), tr("Only whiteboard creator can terminate the whiteboard!"));
}
void MainWindow::OnMsgWhiteboardKickUser(const QString& user) {
    if(whiteboardDialog->isVisible()) {
        FlashTaskbar();
        QMessageBox::warning(this, tr("Kicked"), QString("%1 has removed you from the whiteboard!").arg(user));
        WhiteboardShutdownHelper();
    }
}
void MainWindow::OnMsgWhiteboardDrawLine(const QPoint& start, const QPoint& end, int penWidth, QColor penColor) {
    if(whiteboardDialog->isVisible()) {
        whiteboardDialog->DrawLine(start, end, penWidth, penColor);
    }
}
void MainWindow::OnMsgWhiteboardRequestImage() {
    if(whiteboardDialog->isVisible()) {
        const ImageInfo imgInfo = whiteboardDialog->GetImageInfo();
        MsgStreamWriter streamWriter = clientInfo.client->CreateOutStream(sizeof(QImage::Format) + sizeof(std::size_t) + imgInfo.GetSz(), TYPE_WHITEBOARD, MSG_WHITEBOARD_FORWARD_IMAGE);
        streamWriter.Write(imgInfo.GetFmt());
        streamWriter.Write(imgInfo.GetSz());
        streamWriter.Write(imgInfo.GetData(), static_cast<UINT>(imgInfo.GetSz()));

        clientInfo.client->SendServData(streamWriter);
    }
}
void MainWindow::OnMsgWhiteboardForwardImage(const ImageInfo& imgInfo) {
    if(whiteboardDialog->isVisible()) {
        whiteboardDialog->InitImage(imgInfo);
    }
}
void MainWindow::OnMsgWhiteboardClear(const QColor& clr) {
    if(whiteboardDialog->isVisible()) {
        whiteboardDialog->ClearImage(clr);
    }
}

void MainWindow::OnClientClosedWhiteboard() {
    WhiteboardShutdownHelper();;
}

void MainWindow::OnClientDisconnect(bool unexpected) {
    ClearClients();

    ui->actionDisconnect->setEnabled(false);
    ui->actionConnect->setEnabled(true);
    ui->btnEnter->setEnabled(false);
    ui->menuWhiteboard->setEnabled(false);
    if(unexpected) {
        FlashTaskbar();
        QMessageBox::information(this, tr("Error"), tr("You have been disconnected from server!"));
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
    QMessageBox::warning(this, tr("Error"), QString("File transfer canceled by either you or %1").arg(user));
}
void MainWindow::OnFileReceivedFinished(const QString& user) {
    QMessageBox::information(this, tr("Success"), tr("File transfer finished"));
}
void MainWindow::OnFileReceivedCanceled(const QString& user) {
    QMessageBox::information(this, QString("File transfer canceled by either you or %1").arg(user), tr("File transfer finished"));
}

//returns true if should filter the event
//used so enter triggers enter button click to send message and you can use shift + enter for a newline
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
