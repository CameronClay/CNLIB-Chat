#include "imginfo.h"

ImageInfo::ImageInfo(std::size_t sz, QImage::Format fmt, const std::shared_ptr<uchar[]>& dataOwned)
    :
    sz(sz),
    fmt(fmt),
    dataOwned(dataOwned)
{
    data = this->dataOwned.get();
}
ImageInfo::ImageInfo(std::size_t sz, const uchar* data, QImage::Format fmt)
    :
    sz(sz),
    data(const_cast<uchar*>(data)),
    fmt(fmt),
    dataOwned()
{

}

//ImageInfo::ImageInfo(ImageInfo&& rhs)
//    :
//    sz(rhs.sz),
//    data(rhs.data),
//    fmt(rhs.fmt),
//    dataOwned(std::move(rhs.dataOwned))
//{}

//ImageInfo& ImageInfo::operator=(ImageInfo&& rhs) {
//    if(this != &rhs) {
//        this->sz = rhs.sz;
//        this->data = rhs.data;
//        this->fmt = rhs.fmt;
//        this->dataOwned = std::move(rhs.dataOwned);
//    }

//    return *this;
//}

std::size_t ImageInfo::GetSz() const {
    return sz;
}
const uchar* ImageInfo::GetData() const {
    return data;
}
QImage::Format ImageInfo::GetFmt() const {
    return fmt;
}
