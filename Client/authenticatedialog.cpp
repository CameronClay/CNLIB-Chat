#include "authenticatedialog.h"
#include "ui_authenticatedialog.h"
#include "MessagesExt.h"

AuthenticateDialog::AuthenticateDialog(QWidget *parent, ClientInfo& clientInfo) :
    QDialog(parent),
    clientInfo(clientInfo),
    ui(new Ui::AuthenticateDialog)
{
    ui->setupUi(this);
    ui->txtUsername->setValidator(new QRegularExpressionValidator(QRegularExpression(tr(R"([^:]+)"))));
    ui->txtPassword->setValidator(new QRegularExpressionValidator(QRegularExpression(tr(R"([^:]+)"))));
}

AuthenticateDialog::~AuthenticateDialog()
{
    delete ui;
}

void AuthenticateDialog::on_btnSendCredentials_clicked()
{
    const std::tstring username = ui->txtUsername->text().toStdWString();
    const std::tstring password = ui->txtPassword->text().toStdWString();
    const std::tstring send = username + _T(":") + password;
    clientInfo.user = username;

    auto streamWriter = clientInfo.client->CreateOutStream(StreamWriter::SizeType(send), TYPE_REQUEST, MSG_REQUEST_AUTHENTICATION);
    streamWriter.Write(send);

    clientInfo.client->SendServData(streamWriter);

    this->setVisible(false);
}

void AuthenticateDialog::on_btnCancel_clicked()
{
    clientInfo.Disconnect();
    this->setVisible(false);
}


void AuthenticateDialog::on_txtUsername_textEdited(const QString &arg1)
{
    CondEnableSendBtn();
}


void AuthenticateDialog::on_txtPassword_textEdited(const QString &arg1)
{
    CondEnableSendBtn();
}

bool AuthenticateDialog::CondEnableSendBtn()
{
    const bool enable = ui->txtUsername->text().length() > 0 && ui->txtPassword->text().length() > 0;
    ui->btnSendCredentials->setEnabled(enable);
    return enable;
}

