#include "whiteboard.h"
#include "ui_whiteboard.h"
#include "MessagesExt.h"

#include <QPainter>
#include <QInputDialog>
#include <QColorDialog>

Whiteboard::Whiteboard(QWidget *parent, ClientInfo& clientInfo) :
    QMainWindow(parent),
    ui(new Ui::Whiteboard),
    clientInfo(clientInfo),
    image()
{
    ui->setupUi(this);

    setWindowFlag(Qt::MSWindowsFixedSizeDialogHint, true);

    //setCentralWidget(ui->gridLayoutWidget);
    //setCentralWidget(ui->centralwidget);
    //setCentralWidget(ui->frameDraw);
    //ui->gridLayout->addWidget(ui->frameDraw);

    ui->actionPen->setCheckable(true);
    ui->actionEraser->setCheckable(true);

    //root widget to topleft even when resized
    setAttribute(Qt::WA_StaticContents);
    imgModified = false;
    drawing = false;
    penWidth = 1;
    penColor = Qt::black;
    SetTool(Tool::PEN);

    CreateActions();
}

Whiteboard::~Whiteboard()
{
    delete ui;
}

void Whiteboard::CreateActions() {
    saveAsMenu = new QMenu(this);
    for(const auto& format : QImageWriter::supportedImageFormats()) {
        QString text = tr("%1...").arg(QString(format).toUpper());
        QAction* action = new QAction(text, this);
        action->setData(format);
        //triggered means clicked on
        connect(action, SIGNAL(triggered()), this, SLOT(Save()));
        saveAsActs.append(action);
    }

    for(const auto& action : saveAsActs) {
        saveAsMenu->addAction(action);
    }

    ui->actionSaveAs->setMenu(saveAsMenu);
}

void Whiteboard::on_actionPen_Color_triggered()
{
    const QColor newColor = QColorDialog::getColor(penColor);
    if(newColor.isValid()) {
        penColor = newColor;
    }
}

void Whiteboard::on_actionPen_Width_triggered()
{
    bool ok;
    const int newWidth = QInputDialog::getInt(this, tr("Whiteboard"), tr("Choose pen width: "), penWidth, 1, 50, 1, &ok);
    if(ok) {
        penWidth = newWidth;
    }
}

void Whiteboard::on_actionClear_Screen_triggered()
{
    const QColor newColor = QColorDialog::getColor(backgroundColor);
    ClearImage(newColor);

    auto streamWriter = clientInfo.client->CreateOutStream(sizeof(QColor), TYPE_WHITEBOARD, MSG_WHITEBOARD_CLEAR);
    streamWriter.Write(newColor);
    clientInfo.client->SendServData(streamWriter);
}

void Whiteboard::SetTool(Tool tool) {
    switch(tool) {
    case Whiteboard::Tool::PEN:
        curColor = &penColor;
        ui->actionPen->setChecked(true);
        ui->actionEraser->setChecked(false);
        break;
    case Whiteboard::Tool::ERASER:
        curColor = &backgroundColor;
        ui->actionPen->setChecked(false);
        ui->actionEraser->setChecked(true);
        break;
    }
}

void Whiteboard::on_actionPen_triggered()
{
    SetTool(Tool::PEN);
}

void Whiteboard::on_actionEraser_triggered()
{
    SetTool(Tool::ERASER);
}


void Whiteboard::mousePressEvent(QMouseEvent* event) {
    if(event->button() == Qt::LeftButton) {
        lastPoint = event->pos();
        drawing = true;
    }
}

void Whiteboard::mouseMoveEvent(QMouseEvent* event) {
    if(event->buttons() & Qt::LeftButton && drawing) {
        DrawLineClient(event->pos(), penWidth, *curColor);
    }
}

void Whiteboard::mouseReleaseEvent(QMouseEvent* event) {
    if(event->button() == Qt::LeftButton && drawing) {
        DrawLineClient(event->pos(), penWidth, *curColor);
        drawing = false;
    }
}

void Whiteboard::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
//    auto rect = centralWidget()->rect();
//    rect.setTop(ui->menubar->size().height() + 100);

//    painter.setWindow(rect);
//    painter.setViewport(rect);
    QRect dirtyRect = event->rect();
    painter.drawImage(dirtyRect, image, dirtyRect);
}

void Whiteboard::closeEvent(QCloseEvent *event) {
    clientInfo.client->SendMsg(TYPE_WHITEBOARD, MSG_WHITEBOARD_LEFT);
    emit ClientClosedWhiteboard();
}

void Whiteboard::DrawLine(const QPoint& start, const QPoint& end, std::size_t penWidth, QColor penColor) {
    QPainter painter(&image);
    painter.setPen(QPen(penColor, penWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    if(start == end) {
        painter.drawPoint(start);
    }
    else {
        painter.drawLine(start, end);
    }

    imgModified = true;
    const int rad = (static_cast<int>(penWidth) / 2) + 2;
    update(QRect(start, end).normalized().adjusted(-rad, -rad, rad, rad));
}

void Whiteboard::ClearImage(const QColor& clr) {
    if(clr.isValid()) {
        backgroundColor = clr;
        image.fill(clr);
        imgModified = false;
        update();
    }
}


void Whiteboard::DrawLineClient(const QPoint& endPoint, std::size_t penWidth, QColor penColor) {
    if(wbInviteArgs.canDraw) {
        auto streamWriter = clientInfo.client->CreateOutStream(sizeof(lastPoint) + sizeof(endPoint) + sizeof(penWidth) + sizeof(QRgb), TYPE_WHITEBOARD, MSG_WHITEBOARD_DRAWLINE);
        streamWriter.Write(lastPoint);
        streamWriter.Write(endPoint);
        streamWriter.Write(penWidth);
        streamWriter.Write(penColor.rgb());
        clientInfo.client->SendServData(streamWriter);

        DrawLine(lastPoint, endPoint, penWidth, penColor);
        lastPoint = endPoint;
    }
}

void Whiteboard::ResizeImage(QImage* image, const QSize& newSize) {
    if(image->size() == newSize) {
        return;
    }

    QImage newImage(newSize, QImage::Format_RGB32);
    newImage.fill(qRgb(255, 255, 255));
    QPainter painter(&newImage);
    painter.drawImage(QPoint(0, 0), *image);
    *image = newImage;
}

void Whiteboard::SetInviteParams(const WBInviteParams& inviteArgs) {
    this->wbInviteArgs = inviteArgs;

    const bool enabled = wbInviteArgs.canDraw;
    ui->menuOptions->setEnabled(enabled);
    ui->menuTools->setEnabled(enabled);
}

void Whiteboard::InitBase(const WhiteboardArgs& args) {
    wbArgs = args;

    centralWidget()->setFixedSize(QSize(args.resX, args.resY));
    adjustSize();
    updateGeometry();

    backgroundColor = args.backgroundColor;
}

void Whiteboard::Init(const WhiteboardArgs& args) {
    InitBase(args);

    QSize sz = centralWidget()->size();
    sz.setHeight(sz.height() + ui->menubar->size().height());
    image = QImage(sz, QImage::Format_RGB32);
    image.fill(args.backgroundColor);
    update();
}

void Whiteboard::InitImage(const ImageInfo& imageInfo) {
    const uchar* const data = imageInfo.GetData();
    const auto bytesPerLine = image.bytesPerLine();
    for (int y = 0, height = image.height(); y < height; ++y) {
        memcpy(image.scanLine(y), data + (bytesPerLine * y), bytesPerLine);
    }

    update();
}

void Whiteboard::OnMsgDrawLine(const QPoint& start, const QPoint& end, std::size_t penWidth, QColor penColor) {
    DrawLine(start, end, penWidth, penColor);
}

ImageInfo Whiteboard::GetImageInfo() const {
    ImageInfo imgInfo(image.sizeInBytes(), image.constBits(), image.format());
    return imgInfo;
}

bool Whiteboard::OpenImage(const QString& filename) {
    QImage loadedImage;
    if(!loadedImage.load(filename)) {
        return false;
    }

    //make loaded image same size as current image (canvas)
    image = loadedImage.scaled(size(), Qt::KeepAspectRatioByExpanding);
    imgModified = false;
    update();
    return true;
}

bool Whiteboard::SaveImage(const QString& filename, const char* fileFormat) {
    QImage visibleImage = image;
    ResizeImage(&visibleImage, size());
    if(visibleImage.save(filename, fileFormat)) {
        imgModified = false;
        return true;
    }

    return false;
}

bool Whiteboard::SaveFile(const QByteArray& fileFormat) {
    QString initialPath = QDir::currentPath() + "/untitled." + fileFormat;
    QString filename = QFileDialog::getSaveFileName(this,
                                                    tr("Save As"),
                                                    initialPath,
                                                    tr("%1; Files (*.%2);; All Files(*)").
                                                    arg(QString::fromLatin1(fileFormat.toUpper()), QString::fromLatin1(fileFormat)));
    if(filename.isEmpty()) {
        return false;
    }
    else {
        return SaveImage(filename, fileFormat.constData());
    }
}

bool Whiteboard::MaybeSave() {
    if(imgModified) {
        QMessageBox::StandardButton res;
        res = QMessageBox::warning(
           this,
           tr("Whiteboard"),
           tr("The image has been modified.\n", "Would you like to save your changes?"),
           QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
        );

        if(res == QMessageBox::Save) {
            return SaveFile("pjg");
        }
        else if(res == QMessageBox::Cancel) {
            return false;
        }
    }
    return true;
}

void Whiteboard::Save() {
    QAction* action = qobject_cast<QAction*>(sender());
    QByteArray fileFmt = action->data().toByteArray();
    SaveFile(fileFmt);
}

void Whiteboard::on_actionLoad_triggered() {
    if(MaybeSave()) {
        QString filename = QFileDialog::getOpenFileName(this, tr("Open File"), QDir::currentPath());
        if(!filename.isEmpty()) {
            OpenImage(filename);
        }
    }
}

