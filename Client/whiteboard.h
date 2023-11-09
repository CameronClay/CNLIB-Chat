#ifndef WHITEBOARD_H
#define WHITEBOARD_H

#include <QMainWindow>
#include <QtWidgets>
#include <QColor>
#include <QImage>
#include <QPoint>
#include "imginfo.h"
#include "clientinfo.h"
#include "whiteboardargs.h"
#include "whiteboardinviteparams.h"

namespace Ui {
class Whiteboard;
}

class Whiteboard : public QMainWindow
{
    Q_OBJECT

public:
    explicit Whiteboard(QWidget *parent, ClientInfo& clientInfo);
    ~Whiteboard();

    enum class Tool {
        PEN,
        ERASER
    };

    void Init(const WhiteboardArgs& args);
    void InitImage(const ImageInfo& ImageInfo);
    void SetInviteParams(const WBInviteParams& inviteArgs);

    ImageInfo GetImageInfo() const;

    void SetTool(Tool tool);
    void DrawLine(const QPoint& start, const QPoint& end, std::size_t penWidth, QColor penColor);
    void ClearImage(const QColor& clr);

private slots:
    void on_actionPen_Color_triggered();
    void on_actionPen_Width_triggered();

    void on_actionClear_Screen_triggered();

    void on_actionPen_triggered();
    void on_actionEraser_triggered();

    void on_actionLoad_triggered();

    void Save();

    void OnMsgDrawLine(const QPoint& start, const QPoint& end, std::size_t penWidth, QColor penColor);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    // Updates the paint area
    void paintEvent(QPaintEvent* event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    void CreateActions();

    void InitBase(const WhiteboardArgs& args);
    void ResizeImage(QImage* image, const QSize& newSize);
    void DrawLineClient(const QPoint& endPoint, std::size_t penWidth, QColor penColor);

    bool OpenImage(const QString& filename);
    bool SaveImage(const QString& filename, const char* fileFormat);
    bool SaveFile(const QByteArray& fileFormat);
    bool MaybeSave();

signals:
    void ClientClosedWhiteboard();

private:
    Ui::Whiteboard *ui;

    QMenu* saveAsMenu;
    QList<QAction*> saveAsActs;

    ClientInfo& clientInfo;
    WhiteboardArgs wbArgs;
    WBInviteParams wbInviteArgs;

    bool imgModified;
    bool drawing;
    QColor backgroundColor;
    QColor penColor;
    QColor* curColor;
    std::size_t penWidth;
    QImage image;
    QPoint lastPoint;
};

#endif // WHITEBOARD_H
