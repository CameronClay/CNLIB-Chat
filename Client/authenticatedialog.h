#ifndef AUTHENTICATEDIALOG_H
#define AUTHENTICATEDIALOG_H

#include <QDialog>
#include "clientinfo.h"

namespace Ui {
class AuthenticateDialog;
}

class AuthenticateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AuthenticateDialog(QWidget *parent, ClientInfo& clientInfo);
    ~AuthenticateDialog();

private slots:
    void on_btnSendCredentials_clicked();

    void on_btnCancel_clicked();

    void on_txtUsername_textEdited(const QString &arg1);

    void on_txtPassword_textEdited(const QString &arg1);

private:
    Ui::AuthenticateDialog *ui;
    ClientInfo& clientInfo;

    bool CondEnableSendBtn();

};

#endif // AUTHENTICATEDIALOG_H
