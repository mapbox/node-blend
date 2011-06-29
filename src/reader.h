#ifndef NODE_BLEND_SRC_READER_H
#define NODE_BLEND_SRC_READER_H

#include <png.h>
#include <jpeglib.h>
#include <setjmp.h>
#include <zlib.h>
#include <assert.h>

#include <cstdlib>
#include <cstring>

#include <string>

class ImageReader {
public:
    virtual unsigned char* decode() { return NULL; };
    ImageReader(unsigned char* src, size_t len) : width(0), height(0),
        alpha(false), source(src), length(len), pos(0) {}
    ImageReader(const char* msg) : width(0), height(0), alpha(false),
        message(msg), source(NULL), length(0), pos(0) {}
    virtual ~ImageReader() {};
    static ImageReader* create(unsigned char* surface, size_t len);

    png_uint_32 width;
    png_uint_32 height;
    bool alpha;

    std::string message;
protected:
    unsigned char* source;
    size_t length;
    size_t pos;
};

class PNGImageReader : public ImageReader {
public:
    PNGImageReader(unsigned char* src, size_t len);
    virtual ~PNGImageReader();
    unsigned char* decode();

protected:
    static void readCallback(png_structp png, png_bytep data, png_size_t length);
    static void errorHandler(png_structp png, png_const_charp error_msg);
    static void warningHandler(png_structp png_ptr, png_const_charp warning_msg);

protected:
    int depth;
    int color;
    png_structp png;
    png_infop info;
};


class JPEGImageReader : public ImageReader {
public:
    JPEGImageReader(unsigned char* src, size_t len);
    virtual ~JPEGImageReader();
    unsigned char* decode();

protected:
    struct JPEGErrorManager {
        jpeg_error_mgr pub;
        jmp_buf jump;
        JPEGImageReader* reader;
    };

    static void errorHandler(j_common_ptr cinfo);
    static void errorMessage(j_common_ptr cinfo);

protected:
    jpeg_decompress_struct info;
    JPEGErrorManager err;
};

#endif
