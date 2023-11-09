#ifndef INVITEWHITEBOARDDIALOG_H
#define INVITEWHITEBOARDDIALOG_H

#include <QDialog>
#include "clientinfo.h"
#include "whiteboardinviteparams.h"

namespace Ui {
class InviteWhiteboardDialog;
}

class InviteWhiteboardDialog : public QDialog
{
    Q_OBJECT

public:
    explicit InviteWhiteboardDialog(QWidget *parent, ClientInfo& clientInfo);
    ~InviteWhiteboardDialog();

private slots:
    void on_btnOk_clicked();

    void on_btnCancel_clicked();

signals:
    void InviteWhiteboard(const WBInviteParams& params);

private:
    Ui::InviteWhiteboardDialog *ui;
    ClientInfo& clientInfo;
};

#endif // INVITEWHITEBOARDDIALOG_H
