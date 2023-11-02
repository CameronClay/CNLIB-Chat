#include "readlogdialog.h"
#include "ui_readlogdialog.h"
#include "viewlogdialog.h"

ReadLogDialog::ReadLogDialog(QWidget *parent, ClientInfo& clientInfo) :
    QDialog(parent),
    ui(new Ui::ReadLogDialog),
    clientInfo(clientInfo)
{
    ui->setupUi(this);
    connect(reinterpret_cast<ViewLogDialog*>(parent), &ViewLogDialog::on_setlog, this, &ReadLogDialog::on_setlog);
}

ReadLogDialog::~ReadLogDialog()
{
    delete ui;
}

void ReadLogDialog::on_btnOk_clicked()
{
    this->setVisible(false);
}

void ReadLogDialog::on_btnCancel_clicked()
{
    this->setVisible(false);
}

void ReadLogDialog::on_setlog(const QString& str) {
    ui->txtLog->setText(str);
}

