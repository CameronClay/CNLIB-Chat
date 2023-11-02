#ifndef CONNECTDIALOG_H
#define CONNECTDIALOG_H

#include <QDialog>
#include "clientinfo.h"

namespace Ui {
class ConnectDialog;
}

class ConnectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConnectDialog(QWidget *parent, ClientInfo& clientInfo);
    ~ConnectDialog();

private slots:
    void on_btnConnect_clicked();

    void on_btnCancel_clicked();

    void on_listServer_clicked(const QModelIndex &index);

private:
    Ui::ConnectDialog *ui;
    ClientInfo& clientInfo;
};

#endif // CONNECTDIALOG_H
