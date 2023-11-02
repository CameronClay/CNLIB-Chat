#include "viewlogdialog.h"
#include "ui_viewlogdialog.h"

ViewLogDialog::ViewLogDialog(QWidget *parent, ClientInfo& clientInfo) :
    QDialog(parent),
    clientInfo(clientInfo),
    ui(new Ui::ViewLogDialog)
{
    ui->setupUi(this);

    readLogDialog = std::make_unique<ReadLogDialog>(this, clientInfo);

    const auto logListStd = clientInfo.opts->GetLogList();
    QList<QString> logList;
    std::transform(std::cbegin(logListStd), std::cend(logListStd), std::back_inserter(logList), [](const FileMisc::FileData& filedata){return QString::fromStdWString(filedata.fileName);});
    logsModel = std::make_unique<QStringListModel>(logList, nullptr);
    ui->listLogs->setModel(logsModel.get());
}

ViewLogDialog::~ViewLogDialog()
{
    delete ui;
}

void ViewLogDialog::on_btnClearAll_clicked()
{
    logsModel->removeRows(0, logsModel->rowCount());
    clientInfo.opts->ClearLogs();
}


void ViewLogDialog::on_btnRemove_clicked()
{
    QStringListModel* model = reinterpret_cast<QStringListModel*>(ui->listLogs->model());
    model->removeRows(ui->listLogs->currentIndex().row(), 1);
    clientInfo.opts->RemoveLog(ui->listLogs->currentIndex().row());

    if(model->rowCount() == 0) {
        ui->btnRemove->setEnabled(false);
    }
}


void ViewLogDialog::on_btnOk_clicked()
{
    readLogDialog->setVisible(false);
    this->setVisible(false);
}


void ViewLogDialog::on_btnOpen_clicked()
{
    const auto logIdx = ui->listLogs->currentIndex().row();
    qsizetype nBytes = 0u;
    clientInfo.opts->ReadLog(logIdx, nullptr, reinterpret_cast<DWORD*>(&nBytes));
    char* buffer = alloc<char>(nBytes);
    clientInfo.opts->ReadLog(logIdx, buffer, reinterpret_cast<DWORD*>(&nBytes));

    QString str = QString::fromLocal8Bit(buffer, static_cast<qsizetype>(nBytes));
    emit on_setlog(str);

    readLogDialog->setVisible(true);
}


void ViewLogDialog::on_listLogs_clicked(const QModelIndex &index)
{
    ui->btnRemove->setEnabled(true);
}

