#include "manageserversdialog.h"
#include "ui_manageserversdialog.h"

ManageServersDialog::ManageServersDialog(QWidget *parent, ClientInfo& clientInfo) :
    QDialog(parent),
    clientInfo(clientInfo),
    ui(new Ui::ManageServersDialog)
{
    ui->setupUi(this);

    ui->listServers->setModel(clientInfo.serverListModel.get());

    ui->txtIPv4->setValidator(new QRegularExpressionValidator(QRegularExpression(tr(R"(^(25[0-5]|2[0-4][0-9]|1?[0-9][0-9]{1,2})(\.(25[0-5]|2[0-4][0-9]|1?[0-9]{1,2})){3}$)"))));
    ui->txtIPv6->setValidator(new QRegularExpressionValidator(QRegularExpression(tr(R"(^([0-9a-f]){1,4}(:([0-9a-f]){1,4}){7}$)"))));
    ui->txtPort->setValidator(new QIntValidator(1, 65535, this));

    connect(clientInfo.serverListModel.get(), &QAbstractItemModel::dataChanged , this, &ManageServersDialog::OnListServersModelDataChanged);
    connect(clientInfo.serverListModel.get(), &QAbstractItemModel::rowsInserted, this, &ManageServersDialog::OnListServersModelRowsInserted);
    connect(clientInfo.serverListModel.get(), &QAbstractItemModel::rowsRemoved , this, &ManageServersDialog::OnListServersModelRowsRemoved);
    connect(clientInfo.serverListModel.get(), &QAbstractItemModel::rowsMoved   , this, &ManageServersDialog::OnListServersModelRowsMoved);
}

ManageServersDialog::~ManageServersDialog()
{
    delete ui;
}

void ManageServersDialog::on_btnOk_clicked()
{
    this->setVisible(false);
}


void ManageServersDialog::on_btnCancel_clicked()
{
    this->setVisible(false);
}


void ManageServersDialog::on_btnAdd_clicked()
{
    int len = 0u;
    QString ipv4addr = ui->txtIPv4->text();
    QString ipv6addr = ui->txtIPv6->text();

    if(ui->txtPort->text().length() == 0) {
        ui->lblError->setText("Invalid Port");
        ui->lblError->setVisible(true);
        return;
    }
    QString port = ui->txtPort->text();
    QString addrfull;

    if(ui->txtIPv4->validator()->validate(ipv4addr, len) == QValidator::Acceptable) {
        addrfull = QString("%1:%2").arg(ipv4addr, port);
        ui->lblError->setText("");
        ui->lblError->setVisible(false);
    }
    else if(ui->txtIPv4->validator()->validate(ipv6addr, len) == QValidator::Acceptable) {
        addrfull = QString("[%1]:%2").arg(ipv6addr, port);
        ui->lblError->setText("");
        ui->lblError->setVisible(false);
    }
    else {
        ui->lblError->setText("Invalid IP address");
        ui->lblError->setVisible(true);
        return;
    }

    QStringListModel* model = reinterpret_cast<QStringListModel*>(ui->listServers->model());
    const int row = model->rowCount();
    model->insertRows(row,1);
    model->setData(model->index(row, 0), QVariant(addrfull));

    ui->listServers->setCurrentIndex(model->index(row, 0)); //select new item
    ui->btnRemove->setEnabled(true);

    clientInfo.SaveServList(model->stringList());
}


void ManageServersDialog::on_btnRemove_clicked()
{
    QStringListModel* model = reinterpret_cast<QStringListModel*>(ui->listServers->model());
    model->removeRows(ui->listServers->currentIndex().row(),1);
    clientInfo.SaveServList(model->stringList());

    if(model->rowCount() == 0) {
        ui->btnRemove->setEnabled(false);
    }
}

void ManageServersDialog::on_listServers_clicked(const QModelIndex &index)
{
     ui->btnRemove->setEnabled(true);
}

void ManageServersDialog::OnListServersModelDataChanged(const QModelIndex& topLeft, const QModelIndex & bottomRight)
{
    QStringListModel* model = reinterpret_cast<QStringListModel*>(ui->listServers->model());
    clientInfo.SaveServList(model->stringList());
}

void ManageServersDialog::OnListServersModelRowsInserted(const QModelIndex& parent, int first, int last)
{
    QStringListModel* model = reinterpret_cast<QStringListModel*>(ui->listServers->model());
    clientInfo.SaveServList(model->stringList());
}

void ManageServersDialog::OnListServersModelRowsRemoved(const QModelIndex& parent, int first, int last)
{
    QStringListModel* model = reinterpret_cast<QStringListModel*>(ui->listServers->model());
    clientInfo.SaveServList(model->stringList());
}

void ManageServersDialog::OnListServersModelRowsMoved(const QModelIndex& sourceParent, int sourceStart, int sourceEnd, const QModelIndex& destinationParent, int destinationRow)
{
    QStringListModel* model = reinterpret_cast<QStringListModel*>(ui->listServers->model());
    clientInfo.SaveServList(model->stringList());
}

