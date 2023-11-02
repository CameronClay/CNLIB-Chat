#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>
#include "serverinfo.h"

namespace Ui {
class OptionsDialog;
}

class OptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OptionsDialog(QWidget *parent, ServerInfo& serverInfo);
    ~OptionsDialog();

private slots:
    void on_btnOk_clicked();

    void on_btnCancel_clicked();

    void on_txtPort_textEdited(const QString &arg1);

private:
    Ui::OptionsDialog *ui;
    ServerInfo& serverInfo;
};

#endif // OPTIONSDIALOG_H
