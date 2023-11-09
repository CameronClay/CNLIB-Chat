#ifndef IMGINFO_H
#define IMGINFO_H
#include <QtWidgets>
#include <cstddef>
#include <memory>

class ImageInfo {
public:
    ImageInfo(std::size_t sz, QImage::Format fmt, const std::shared_ptr<uchar[]>& dataOwned);
    ImageInfo(std::size_t sz, const uchar* data, QImage::Format fmt);
//    ImageInfo(ImageInfo&& rhs);
//    ImageInfo& operator=(ImageInfo&& rhs);

    std::size_t GetSz() const;
    const uchar* GetData() const;
    QImage::Format GetFmt() const;

private:
    std::size_t sz;
    uchar* data;
    QImage::Format fmt;
    std::shared_ptr<uchar[]> dataOwned;
};

#endif // IMGINFO_H
