#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "serverinfo.h"
#include "manageadminsdialog.h"
#include "optionsdialog.h"

#include <memory>
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void InitDialogs();
    void InitHandlers();
    void InitMenus();

private slots:
    void on_actionOptions_triggered();

    void on_actionManage_Admins_triggered();

    void on_actionAbout_Qt_triggered();

    void on_actionAbout_triggered();

    void OnUpdateLog();

private:
    Ui::MainWindow *ui;
    ServerInfo serverInfo;
    std::unique_ptr<ManageAdminsDialog> manageAdminsDialog;
    std::unique_ptr<OptionsDialog> optionsDialog;
};
#endif // MAINWINDOW_H
