#include "optionsdialog.h"
#include "ui_optionsdialog.h"

OptionsDialog::OptionsDialog(QWidget *parent, ServerInfo& serverInfo) :
    QDialog(parent),
    serverInfo(serverInfo),
    ui(new Ui::OptionsDialog)
{
    ui->setupUi(this);
    ui->txtPort->setValidator(new QIntValidator(1, 65535, this));
    ui->txtPort->setText(QString::number(serverInfo.port));
}

OptionsDialog::~OptionsDialog()
{
    delete ui;
}

void OptionsDialog::on_btnOk_clicked()
{
    serverInfo.port = ui->txtPort->text().toUShort();
    serverInfo.SaveOptions();
    this->setVisible(false);
}


void OptionsDialog::on_btnCancel_clicked()
{
    this->setVisible(false);
}


void OptionsDialog::on_txtPort_textEdited(const QString &arg1)
{

}

