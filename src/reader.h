#ifndef NODE_BLEND_SRC_READER_H
#define NODE_BLEND_SRC_READER_H

#include <png.h>
#include <zlib.h>
#include <assert.h>

class ImageReader {
public:
    inline unsigned long getWidth() { return width; }
    inline unsigned long getheight() { return height; }
    inline bool getAlpha() { return alpha; }
    virtual unsigned char* decode() = 0;
    ImageReader(const unsigned char* src, size_t len) : width(0), height(0),
        depth(0), color(-1), alpha(false), source(src), length(len), pos(0) {}
    virtual ~ImageReader();
    static ImageReader* create(const unsigned char* surface, size_t len);

    unsigned long width;
    unsigned long height;
    int depth;
    int color;
    bool alpha;
protected:
    const unsigned char* source;
    size_t length;
    size_t pos;
};

class PNGImageReader : public ImageReader {
public:
    PNGImageReader(const unsigned char* src, size_t len);
    virtual ~PNGImageReader();
    unsigned char* decode();

protected:
    static void readCallback(png_structp png, png_bytep data, png_size_t length);

protected:
    png_structp png;
    png_infop info;
};


class JPEGImageReader : public ImageReader {
public:
    JPEGImageReader(const unsigned char* src, size_t len);
    virtual ~JPEGImageReader();
    unsigned char* decode();

protected:

protected:
};

#endif
