#include "connectdialog.h"
#include "ui_connectdialog.h"
#include "MessagesExt.h"

ConnectDialog::ConnectDialog(QWidget *parent, ClientInfo& clientInfo) :
    QDialog(parent),
    clientInfo(clientInfo),
    ui(new Ui::ConnectDialog)
{
    ui->setupUi(this);

    ui->listServer->setModel(clientInfo.serverListModel.get());
}

ConnectDialog::~ConnectDialog()
{
    delete ui;
}

void ConnectDialog::on_btnConnect_clicked()
{
    QStringListModel* model = reinterpret_cast<QStringListModel*>(ui->listServer->model());
    std::wstring ip = model->data(ui->listServer->currentIndex()).toString().toStdWString();
    const UINT pos = ip.find_last_of(_T(":"));
    clientInfo.Connect(ip.substr(0, pos).c_str(), ip.substr(pos + 1).c_str(), ip[0] == _T('[') ? true : false, clientInfo.timeOut);
    if (clientInfo.client->IsConnected())
    {
        if (clientInfo.client->RecvServData(2, 1))
        {
            auto streamWriter = clientInfo.client->CreateOutStream(sizeof(float), TYPE_VERSION, MSG_VERSION_CHECK);
            streamWriter.Write(Options::APPVERSION);

            clientInfo.client->SendServData(streamWriter);
        }
        this->setVisible(false);
    }
    else
    {
        QMessageBox::warning(this, tr("ERROR"), tr("Unable to connect to server!"));
    }
}


void ConnectDialog::on_btnCancel_clicked()
{
    this->setVisible(false);
}


void ConnectDialog::on_listServer_clicked(const QModelIndex &index)
{
    ui->btnConnect->setEnabled(true);
}

