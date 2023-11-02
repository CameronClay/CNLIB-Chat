#include "StdAfx.h"
#include "Options.h"
#include <QCoreApplication>
#include <QSettings>
#include <QStandardPaths>
#include <QFile>
#include <QDir>

Options::Options(const QString& optionsDir, const QString& downloadDir)
{
    Reset(optionsDir, downloadDir);
}

Options::Options(Options&& opts)
    :
    Logs(std::move(opts)),
    optionsDir(std::move(opts.optionsDir)),
    downloadDir(std::move(opts.downloadDir)),
    optionsPath(std::move(opts.optionsPath)),
    version(opts.version),
    timeStamps(opts.timeStamps),
    startUp(opts.startUp),
    flashTaskbar(opts.flashTaskbar),
    saveLogs(opts.saveLogs),
    flashCount(opts.flashCount),
    fontFamily(std::move(opts.fontFamily)),
    fontSize(opts.fontSize)
{}

Options::~Options()
{}

void Options::Load(const LIB_TCHAR* windowName)
{
    QFile file(optionsPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QDataStream in(&file);
    in.setByteOrder(QDataStream::ByteOrder::BigEndian);
    //in.setEncoding(QStringConverter::Utf8);

    in >> downloadDir
        >> startUp
        >> flashTaskbar
        >> timeStamps
        >> saveLogs
        >> flashCount
        >> fontFamily
        >> fontSize;
    file.close();

    const std::tstring logsDirTStr = logsDir.toStdWString();
    LoadLogList(logsDirTStr.c_str());

    if (SaveLogs()) {
        CreateLog();
    }
}

void Options::Save(const LIB_TCHAR* windowName)
{
    QFile file(optionsPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }

    QDataStream out(&file);
    out.setByteOrder(QDataStream::ByteOrder::BigEndian);
    //out.setEncoding(QStringConverter::Utf8);

    out << downloadDir
        << startUp
        << flashTaskbar
        << timeStamps
        << saveLogs
        << flashCount
        << fontFamily
        << fontSize;
    file.close();

    StartWithOS(startUp, windowName);
}

void Options::StartWithOS(bool startUp, const LIB_TCHAR* windowName) {
#ifdef Q_OS_WIN
    //HKEY_LOCAL_MACHINE requires admin privileges
    QSettings regSettings("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    if(startUp) {
        QString appPath = QCoreApplication::applicationFilePath();
        regSettings.setValue(windowName, appPath);
    }
    else {
        regSettings.remove(windowName);
    }

    regSettings.sync();
#endif
}

void Options::SetGeneral(bool timeStamps, bool startUp, bool flashTaskbar, bool saveLogs, std::size_t flashCount)
{
    this->timeStamps   = timeStamps;
    this->startUp      = startUp;
	this->flashTaskbar = flashTaskbar;
    this->saveLogs     = saveLogs;
    this->flashCount   = flashCount;
}

void Options::SetFont(const QString& fontFamily, int fontSize) {
    this->fontFamily = fontFamily;
    this->fontSize   = fontSize;
}
const QString& Options::GetFontFamily() const {
    return fontFamily;
}
int Options::GetFontSize() const {
    return fontSize;
}

void Options::SetDownloadDir(const QString& dir)
{
    downloadDir = dir;
}

void Options::Reset(const QString& optionsDir, const QString& downloadDir)
{
    timeStamps     = true;
    startUp        = false;
    flashTaskbar   = true;
    saveLogs       = true;
    flashCount     = 3;
    version        = Options::CONFIGVERSION;

    this->optionsDir   = optionsDir;
    this->downloadDir  = downloadDir;
    this->logsDir      = QString("%1/%2").arg(optionsDir, QString("Logs"));
    this->optionsPath  = QString("%1/%2").arg(optionsDir, Options::OPTIONS_FILENAME);

    fontFamily     = QString("Segoe UI");
    fontSize       = 9;

    if(!QDir(optionsDir).exists()) {
       QDir().mkpath(optionsDir);
    }
    if(!QDir(logsDir).exists()) {
       QDir().mkpath(logsDir);
    }
}

bool Options::TimeStamps() const
{
	return timeStamps;
}

bool Options::AutoStartup() const
{
	return startUp;
}

bool Options::SaveLogs() const
{
	return saveLogs;
}

bool Options::FlashTaskbar() const
{
    return flashTaskbar;
}

std::size_t Options::GetFlashCount() const
{
	return flashCount;
}

const QString& Options::GetDownloadDir() const
{
    return downloadDir;
}
