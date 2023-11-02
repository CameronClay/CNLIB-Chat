#ifndef READLOGDIALOG_H
#define READLOGDIALOG_H

#include <QDialog>
#include "clientinfo.h"

namespace Ui {
class ReadLogDialog;
}

class ReadLogDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReadLogDialog(QWidget *parent, ClientInfo& clientInfo);
    ~ReadLogDialog();

private slots:
    void on_btnOk_clicked();

    void on_btnCancel_clicked();

    void on_setlog(const QString& str);

private:
    Ui::ReadLogDialog *ui;
    ClientInfo& clientInfo;
};

#endif // READLOGDIALOG_H
