#ifndef MANAGEADMINSDIALOG_H
#define MANAGEADMINSDIALOG_H

#include <QDialog>
#include <QTWidgets>
#include <memory>
#include "serverinfo.h"

namespace Ui {
class ManageAdminsDialog;
}

class ManageAdminsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ManageAdminsDialog(QWidget *parent, ServerInfo& serverInfo);
    ~ManageAdminsDialog();

private slots:
    void on_btnRemove_clicked();

    void on_btnAdd_clicked();

    void on_btnOk_clicked();

    void on_btnCancel_clicked();

    void on_listAdmins_clicked(const QModelIndex &index);

    void on_txtAdmin_textEdited(const QString &arg1);

    void OnListAdminsModelDataChanged(const QModelIndex& topLeft, const QModelIndex & bottomRight);
    void OnListAdminsModelRowsInserted(const QModelIndex& parent, int first, int last);
    void OnListAdminsModelRowsRemoved(const QModelIndex& parent, int first, int last);
    void OnListAdminsModelRowsMoved(const QModelIndex& sourceParent, int sourceStart, int sourceEnd, const QModelIndex& destinationParent, int destinationRow);

private:
    Ui::ManageAdminsDialog *ui;
    ServerInfo& serverInfo;
};

#endif // MANAGEADMINSDIALOG_H
