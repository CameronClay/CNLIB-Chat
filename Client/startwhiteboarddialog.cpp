#include "startwhiteboarddialog.h"
#include "ui_startwhiteboarddialog.h"
#include <QColorDialog>

StartWhiteboardDialog::StartWhiteboardDialog(QWidget *parent, ClientInfo& clientInfo) :
    QDialog(parent),
    ui(new Ui::StartWhiteboardDialog),
    clientInfo(clientInfo)
{
    ui->setupUi(this);

    ui->txtResX->setValidator(new QRegularExpressionValidator(QRegularExpression(tr(R"([0-9]*)"))));
    ui->txtResY->setValidator(new QRegularExpressionValidator(QRegularExpression(tr(R"([0-9]*)"))));

    backgroundColor = Qt::white;
    SetBtnColorColor(*backgroundColor);
}

StartWhiteboardDialog::~StartWhiteboardDialog()
{
    delete ui;
}

void StartWhiteboardDialog::on_btnOk_clicked()
{
    this->setVisible(false);
    emit StartWhiteboard(WhiteboardArgs{*backgroundColor, ui->txtResX->text().toULongLong(), ui->txtResY->text().toULongLong()});
}


void StartWhiteboardDialog::on_btnCancel_clicked()
{
   this->setVisible(false);
}


void StartWhiteboardDialog::on_btnBackColor_clicked()
{
   backgroundColor = QColorDialog::getColor(Qt::black, this, tr("Choose background color..."));
   SetBtnColorColor(*backgroundColor);

   EnableOk();
}


void StartWhiteboardDialog::on_txtResX_textEdited(const QString &arg1)
{
   EnableOk();
}


void StartWhiteboardDialog::on_txtResY_textEdited(const QString &arg1)
{
   EnableOk();
}

void StartWhiteboardDialog::EnableOk() {
   const bool enabled = ui->txtResX->text().length() > 0 && ui->txtResY->text().length() && backgroundColor;
   ui->btnOk->setEnabled(enabled);
}

void StartWhiteboardDialog::SetBtnColorColor(const QColor& clr) {
   QPalette pal = ui->btnBackColor->palette();
   pal.setColor(QPalette::Button, clr);
   ui->btnBackColor->setAutoFillBackground(true);
   ui->btnBackColor->setPalette(pal);
   ui->btnBackColor->update();
}
