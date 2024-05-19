#ifndef OPTIONS_H
#define OPTIONS_H

#include "Logs.h"
#include <QString>

class Options : public Logs
{
public:
    static constexpr float APPVERSION              = 1.0f;
    static constexpr float CONFIGVERSION           = .0050f;

    static constexpr LIB_TCHAR FOLDER_NAME[]       = _T("Cam's-Client");
    static constexpr LIB_TCHAR SERVLIST_FILENAME[] = _T("ServrecvList.txt");
    static constexpr LIB_TCHAR OPTIONS_FILENAME[]  = _T("Options.dat");

    Options(const QString& optionsDir, const QString& downloadDir);
	Options(Options&& opts);
	~Options();

	void Load(const LIB_TCHAR* windowName);
	void Save(const LIB_TCHAR* windowName);
    void Reset(const QString& optionsDir, const QString& downloadDir);
    static void StartWithOS(bool startUp, const LIB_TCHAR* windowName);

    void SetFont(const QString& fontFamily, int fontSize);
    void SetGeneral(bool timeStamps, bool startUp, bool flashTaskbar, bool saveLogs, std::size_t flashCount);
    void SetDownloadDir(const QString& dir);

    const QString& GetFontFamily() const;
    int GetFontSize() const;
	bool TimeStamps() const;
	bool AutoStartup() const;
	bool SaveLogs() const;
    bool FlashTaskbar() const;
    std::size_t GetFlashCount() const;
    const QString& GetDownloadDir() const;
private:
    QString optionsDir;
    QString logsDir;
    QString downloadDir;
    QString optionsPath;
	float version;
    bool timeStamps, startUp, flashTaskbar, saveLogs;
    std::size_t flashCount;

    QString fontFamily;
    int fontSize;
};

#endif
