#ifndef MANAGESERVERSDIALOG_H
#define MANAGESERVERSDIALOG_H

#include <QDialog>
#include "clientinfo.h"

namespace Ui {
class ManageServersDialog;
}

class ManageServersDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ManageServersDialog(QWidget *parent, ClientInfo& clientInfo);
    ~ManageServersDialog();

private slots:
    void on_btnOk_clicked();

    void on_btnCancel_clicked();

    void on_btnAdd_clicked();

    void on_btnRemove_clicked();

    void on_listServers_clicked(const QModelIndex &index);

    void OnListServersModelDataChanged(const QModelIndex& topLeft, const QModelIndex & bottomRight);
    void OnListServersModelRowsInserted(const QModelIndex& parent, int first, int last);
    void OnListServersModelRowsRemoved(const QModelIndex& parent, int first, int last);
    void OnListServersModelRowsMoved(const QModelIndex& sourceParent, int sourceStart, int sourceEnd, const QModelIndex& destinationParent, int destinationRow);

private:
    Ui::ManageServersDialog *ui;
    ClientInfo& clientInfo;
};

#endif // MANAGESERVERSDIALOG_H
