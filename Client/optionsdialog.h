#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H
#include "clientinfo.h"

#include <QDialog>

namespace Ui {
class OptionsDialog;
}

class OptionsDialog : public QDialog
{
    Q_OBJECT

public:
    friend class MainWindow;

    explicit OptionsDialog(QWidget *parent, ClientInfo& clientInfo);
    ~OptionsDialog();

private slots:
    void on_chkTimeStamps_stateChanged(int arg1);

    void on_chkStartWithWindows_stateChanged(int arg1);

    void on_chkFlashTaskbar_stateChanged(int arg1);

    void on_chkSaveLogs_stateChanged(int arg1);

    void on_txtFlashCount_textEdited(const QString &arg1);

    void on_btnOk_clicked();

    void on_btnCancel_clicked();

    void on_comboBoxFont_currentFontChanged(const QFont &f);

    void on_btnDownloadPath_clicked();

    void on_txtFontSize_textEdited(const QString &arg1);

signals:
    void OnFontChanged(const QFont& font);

private:
    Ui::OptionsDialog *ui;
    ClientInfo& clientInfo;
    QFont font;

    void SetFromUI();
};

#endif // OPTIONSDIALOG_H
