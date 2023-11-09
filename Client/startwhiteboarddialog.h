#ifndef STARTWHITEBOARDDIALOG_H
#define STARTWHITEBOARDDIALOG_H

#include <QDialog>
#include <QColor>
#include <optional>
#include "clientinfo.h"
#include "whiteboardargs.h"

namespace Ui {
class StartWhiteboardDialog;
}

class StartWhiteboardDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StartWhiteboardDialog(QWidget *parent, ClientInfo& clientInfo);
    ~StartWhiteboardDialog();

private slots:
    void on_btnOk_clicked();

    void on_btnCancel_clicked();

    void on_btnBackColor_clicked();

    void on_txtResX_textEdited(const QString &arg1);

    void on_txtResY_textEdited(const QString &arg1);

signals:
    void StartWhiteboard(const WhiteboardArgs& args);

private:
    void EnableOk();
    void SetBtnColorColor(const QColor& clr);

private:
    Ui::StartWhiteboardDialog *ui;
    ClientInfo& clientInfo;

    std::optional<QColor> backgroundColor;
};

#endif // STARTWHITEBOARDDIALOG_H
