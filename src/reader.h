#ifndef NODE_BLEND_SRC_READER_H
#define NODE_BLEND_SRC_READER_H

#include <png.h>
#include <jpeglib.h>
#include <zlib.h>
#include <assert.h>

class ImageReader {
public:
    inline unsigned long getWidth() { return width; }
    inline unsigned long getheight() { return height; }
    inline bool getAlpha() { return alpha; }
    virtual unsigned char* decode() = 0;
    ImageReader(unsigned char* src, size_t len) : width(0), height(0),
        alpha(false), source(src), length(len), pos(0) {}
    static ImageReader* create(unsigned char* surface, size_t len);

    unsigned long width;
    unsigned long height;
    bool alpha;
protected:
    unsigned char* source;
    size_t length;
    size_t pos;
};

class PNGImageReader : public ImageReader {
public:
    PNGImageReader(unsigned char* src, size_t len);
    unsigned char* decode();

protected:
    static void readCallback(png_structp png, png_bytep data, png_size_t length);

protected:
    int depth;
    int color;
    png_structp png;
    png_infop info;
};


class JPEGImageReader : public ImageReader {
public:
    JPEGImageReader(unsigned char* src, size_t len);
    unsigned char* decode();

protected:

protected:
    jpeg_decompress_struct info;
	jpeg_error_mgr err;
};

#endif
