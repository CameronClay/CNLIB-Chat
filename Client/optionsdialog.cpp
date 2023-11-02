#include "optionsdialog.h"
#include "ui_optionsdialog.h"

OptionsDialog::OptionsDialog(QWidget *parent, ClientInfo& clientInfo) :
    QDialog(parent),
    clientInfo(clientInfo),
    ui(new Ui::OptionsDialog)
{
    ui->setupUi(this);
    ui->txtFlashCount->setValidator(new QIntValidator(0, 100, this));
    ui->txtFontSize->setValidator(new QIntValidator(1, 72, this));
    SetFromUI();
}

OptionsDialog::~OptionsDialog()
{
    delete ui;
}

void OptionsDialog::SetFromUI() {
    ui->chkSaveLogs->setChecked(clientInfo.opts->SaveLogs());
    ui->chkFlashTaskbar->setChecked(clientInfo.opts->FlashTaskbar());
    ui->chkStartWithWindows->setChecked(clientInfo.opts->AutoStartup());
    ui->chkTimeStamps->setChecked(clientInfo.opts->TimeStamps());

    ui->lblDownloadPath->setText(clientInfo.opts->GetDownloadDir());
    ui->txtFlashCount->setText(QString::number(clientInfo.opts->GetFlashCount()));

    font = QFont(clientInfo.opts->GetFontFamily(), clientInfo.opts->GetFontSize());
    ui->comboBoxFont->setFont(font);
    ui->txtFontSize->setText(QString::number(clientInfo.opts->GetFontSize()));
    emit OnFontChanged(font);
}

void OptionsDialog::on_chkTimeStamps_stateChanged(int arg1)
{

}


void OptionsDialog::on_chkStartWithWindows_stateChanged(int arg1)
{

}


void OptionsDialog::on_chkFlashTaskbar_stateChanged(int arg1)
{

}


void OptionsDialog::on_chkSaveLogs_stateChanged(int arg1)
{

}


void OptionsDialog::on_txtFlashCount_textEdited(const QString &arg1)
{

}


void OptionsDialog::on_btnOk_clicked()
{
    const bool prevSaveLogs = clientInfo.opts->SaveLogs();
    clientInfo.opts->SetGeneral(ui->chkTimeStamps->checkState(), ui->chkStartWithWindows->checkState(), ui->chkFlashTaskbar->checkState(), ui->chkSaveLogs->checkState(), ui->txtFlashCount->text().toULongLong());

    if (!prevSaveLogs && clientInfo.opts->SaveLogs())
    {
        clientInfo.opts->CreateLog();
        clientInfo.opts->WriteLine(clientInfo.textBuffer.GetText());
    }

    if (prevSaveLogs && !clientInfo.opts->SaveLogs()) {
        clientInfo.opts->RemoveTempLog();
    }
    const std::size_t fontSize = ui->txtFontSize->text().toULongLong();
    font.setPointSize(fontSize);
    clientInfo.opts->SetFont(font.family(), fontSize);
    emit OnFontChanged(font);

    clientInfo.opts->Save(ClientInfo::WINDOW_NAME);

    this->setVisible(false);
}


void OptionsDialog::on_btnCancel_clicked()
{
    SetFromUI();
    this->setVisible(false);
}


void OptionsDialog::on_comboBoxFont_currentFontChanged(const QFont &f)
{
    font = f;
}


void OptionsDialog::on_btnDownloadPath_clicked()
{
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Choose download path"), QDir::currentPath());
    clientInfo.opts->SetDownloadDir(dir);
    ui->lblDownloadPath->setText(dir);
}


void OptionsDialog::on_txtFontSize_textEdited(const QString &arg1)
{
    ui->btnOk->setEnabled(arg1.length() > 0);
}

