#ifndef VIEWLOGDIALOG_H
#define VIEWLOGDIALOG_H

#include <QDialog>
#include <memory>
#include "clientinfo.h"
#include "readlogdialog.h"

namespace Ui {
class ViewLogDialog;
}

class ViewLogDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ViewLogDialog(QWidget *parent, ClientInfo& clientInfo);
    ~ViewLogDialog();

private slots:
    void on_btnClearAll_clicked();

    void on_btnRemove_clicked();

    void on_btnOk_clicked();

    void on_btnOpen_clicked();

    void on_listLogs_clicked(const QModelIndex &index);

signals:
    void on_setlog(const QString& str);

private:
    Ui::ViewLogDialog *ui;
    ClientInfo& clientInfo;
    std::unique_ptr<QStringListModel> logsModel;
    std::unique_ptr<ReadLogDialog> readLogDialog;
};

#endif // VIEWLOGDIALOG_H
