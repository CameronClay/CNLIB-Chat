#include "manageadminsdialog.h"
#include "ui_manageadminsdialog.h"

ManageAdminsDialog::ManageAdminsDialog(QWidget *parent, ServerInfo& serverInfo) :
    QDialog(parent),
    serverInfo(serverInfo),
    ui(new Ui::ManageAdminsDialog)
{
    ui->setupUi(this);
    ui->listAdmins->setModel(serverInfo.adminsModel.get());
    ui->listAdmins->setAcceptDrops(true);
    ui->listAdmins->setDragEnabled(true);
    ui->listAdmins->setDragDropMode(QAbstractItemView::InternalMove);
    ui->listAdmins->setDefaultDropAction(Qt::MoveAction);

    connect(serverInfo.adminsModel.get(), &QAbstractItemModel::dataChanged , this, &ManageAdminsDialog::OnListAdminsModelDataChanged);
    connect(serverInfo.adminsModel.get(), &QAbstractItemModel::rowsInserted, this, &ManageAdminsDialog::OnListAdminsModelRowsInserted);
    connect(serverInfo.adminsModel.get(), &QAbstractItemModel::rowsRemoved , this, &ManageAdminsDialog::OnListAdminsModelRowsRemoved);
    connect(serverInfo.adminsModel.get(), &QAbstractItemModel::rowsMoved   , this, &ManageAdminsDialog::OnListAdminsModelRowsMoved);
}

ManageAdminsDialog::~ManageAdminsDialog()
{
    delete ui;
}

void ManageAdminsDialog::on_btnRemove_clicked()
{
    QStringListModel* model = reinterpret_cast<QStringListModel*>(ui->listAdmins->model());
    model->removeRows(ui->listAdmins->currentIndex().row(),1);
    serverInfo.SaveAdminList(model->stringList());

    if(model->rowCount() == 0) {
        ui->btnRemove->setEnabled(false);
    }
}


void ManageAdminsDialog::on_btnAdd_clicked()
{
    QStringListModel* model = reinterpret_cast<QStringListModel*>(ui->listAdmins->model());
    const int row = model->rowCount();
    model->insertRows(row,1);
    model->setData(model->index(row, 0), QVariant(ui->txtAdmin->text()));
    ui->listAdmins->setCurrentIndex(model->index(row, 0)); //select new item
    serverInfo.SaveAdminList(model->stringList());

    ui->txtAdmin->setText(tr(""));
    ui->btnAdd->setEnabled(false);
    ui->btnRemove->setEnabled(true);
}


void ManageAdminsDialog::on_btnOk_clicked()
{
    this->setVisible(false);
}


void ManageAdminsDialog::on_btnCancel_clicked()
{
    this->setVisible(false);
}


void ManageAdminsDialog::on_listAdmins_clicked(const QModelIndex &index)
{
     ui->btnRemove->setEnabled(true);
}


void ManageAdminsDialog::on_txtAdmin_textEdited(const QString &arg1)
{
     if(ui->txtAdmin->text().length() > 0) {
         ui->btnAdd->setEnabled(true);
     }
}

void ManageAdminsDialog::OnListAdminsModelDataChanged(const QModelIndex& topLeft, const QModelIndex & bottomRight) {
    QStringListModel* model = reinterpret_cast<QStringListModel*>(ui->listAdmins->model());
    serverInfo.SaveAdminList(model->stringList());
}
void ManageAdminsDialog::OnListAdminsModelRowsInserted(const QModelIndex& parent, int first, int last) {
    QStringListModel* model = reinterpret_cast<QStringListModel*>(ui->listAdmins->model());
    serverInfo.SaveAdminList(model->stringList());
}
void ManageAdminsDialog::OnListAdminsModelRowsRemoved(const QModelIndex& parent, int first, int last) {
    QStringListModel* model = reinterpret_cast<QStringListModel*>(ui->listAdmins->model());
    serverInfo.SaveAdminList(model->stringList());
}
void ManageAdminsDialog::OnListAdminsModelRowsMoved(const QModelIndex& sourceParent, int sourceStart, int sourceEnd, const QModelIndex& destinationParent, int destinationRow) {
    QStringListModel* model = reinterpret_cast<QStringListModel*>(ui->listAdmins->model());
    serverInfo.SaveAdminList(model->stringList());
}

