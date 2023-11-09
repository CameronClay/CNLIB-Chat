#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QtWidgets>
#include <mutex>
#include <cassert>

#include "StdAfx.h"
#include "CNLIB/Typedefs.h"
#include "CNLIB/TCPServInterface.h"
#include "CNLIB/MsgStream.h"
#include "CNLIB/File.h"
#include "CNLIB/UPNP.h"
#include "MessagesExt.h"
#include "Format.h"
#include "TextDisplay.h"

//Built using QT

MainWindow::MainWindow(QWidget *parent)
    :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    serverInfo()
{
    ui->setupUi(this);

    InitDialogs();
    InitMenus();
    InitHandlers();
}

void MainWindow::InitDialogs() {
    manageAdminsDialog = std::make_unique<ManageAdminsDialog>(this, serverInfo);
    optionsDialog = std::make_unique<OptionsDialog>(this, serverInfo);
}

void MainWindow::InitHandlers() {
    connect(&serverInfo, &ServerInfo::UpdateLog, this, &MainWindow::OnUpdateLog);
}

void MainWindow::InitMenus() {

}

MainWindow::~MainWindow() {
    DestroyServer(serverInfo.serv);
    delete ui;
}


void MainWindow::on_actionOptions_triggered() {
    optionsDialog->show();
}


void MainWindow::on_actionManage_Admins_triggered() {
    manageAdminsDialog->show();
}

void MainWindow::on_actionAbout_triggered() {
    QMessageBox::about(this, tr("About Server"), tr("TCP chat server built using WNLIB and QT, similar to IRC."));
}

void MainWindow::on_actionAbout_Qt_triggered() {
    QMessageBox::aboutQt(this, tr("About Qt"));
}

void MainWindow::OnUpdateLog() {
    ui->txtDisp->setText(QString::fromStdWString(serverInfo.textBuffer.GetText()));
}

