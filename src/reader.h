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
    virtual void decode(unsigned char* surface, bool alpha = true) = 0;
    ImageReader() : width(0), height(0), depth(0), color(-1), alpha(false),
                    source(NULL), length(0), pos(0) {}
    virtual ~ImageReader();
    static ImageReader* create(const char* surface, size_t len);

    png_uint_32 width;
    png_uint_32 height;
    int depth;
    int color;
    bool alpha;
protected:
    const char* source;
    size_t length;
    size_t pos;
};

class PNGImageReader : public ImageReader {
public:
    PNGImageReader(const char* src, size_t len);
    virtual ~PNGImageReader();
    void decode(unsigned char* surface, bool alpha);

protected:
    static void readCallback(png_structp png, png_bytep data, png_size_t length);

protected:
    png_structp png;
    png_infop info;
};

#endif
