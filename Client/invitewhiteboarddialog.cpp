#include "invitewhiteboarddialog.h"
#include "ui_invitewhiteboarddialog.h"

InviteWhiteboardDialog::InviteWhiteboardDialog(QWidget *parent, ClientInfo& clientInfo) :
    QDialog(parent),
    ui(new Ui::InviteWhiteboardDialog),
    clientInfo(clientInfo)
{
    ui->setupUi(this);
}

InviteWhiteboardDialog::~InviteWhiteboardDialog()
{
    delete ui;
}

void InviteWhiteboardDialog::on_btnOk_clicked()
{
    this->setVisible(false);
    WBInviteParams inviteParams{ui->chkCanDraw->checkState() == Qt::CheckState::Checked};
    emit InviteWhiteboard(inviteParams);
}


void InviteWhiteboardDialog::on_btnCancel_clicked()
{
    this->setVisible(false);
}

