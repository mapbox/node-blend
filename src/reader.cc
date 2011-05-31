#include "reader.h"

PNGImageReader::PNGImageReader(const unsigned char* src, size_t len) :
        ImageReader(src, len) {
    // Decode PNG header.
    png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    assert(png);
    info = png_create_info_struct(png);
    assert(info);
    png_set_read_fn(png, this, readCallback);
    png_read_info(png, info);
    png_get_IHDR(png, info, &width, &height, &depth, &color, NULL, NULL, NULL);
    alpha = (color & PNG_COLOR_MASK_ALPHA) || png_get_valid(png, info, PNG_INFO_tRNS);
}

void PNGImageReader::readCallback(png_structp png, png_bytep data, png_size_t length) {
    PNGImageReader* reader = static_cast<PNGImageReader*>(png_get_io_ptr(png));

    // Read `length` bytes into `data`.
    if (reader->pos + length > reader->length) {
        png_error(png, "Read Error");
        return;
    }

    memcpy(data, reader->source + reader->pos, length);
    reader->pos += length;
}


unsigned char* PNGImageReader::decode() {
    // From http://trac.mapnik.org/browser/trunk/src/png_reader.cpp
    if (color == PNG_COLOR_TYPE_PALETTE)
        png_set_expand(png);
    if (color == PNG_COLOR_TYPE_GRAY)
        png_set_expand(png);
    if (png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_expand(png);
    if (depth == 16)
        png_set_strip_16(png);
    if (depth < 8)
        png_set_packing(png);
    if (color == PNG_COLOR_TYPE_GRAY ||
        color == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png);

    // Always add an alpha channel.
    if (!this->alpha) {
        png_set_add_alpha(png, 0xFF, PNG_FILLER_AFTER);
    }

    double gamma;
    if (png_get_gAMA(png, info, &gamma))
        png_set_gamma(png, 2.2, gamma);

    png_read_update_info(png, info);

    unsigned int rowbytes = png_get_rowbytes(png, info);
    assert(width * 4 == rowbytes);

    unsigned char* surface = (unsigned char*)malloc(width * height * 4);
    assert(surface);

    png_bytep row_pointers[height];
    for (unsigned i = 0; i < height; i++) {
        row_pointers[i] = (unsigned char*)(surface + (i * rowbytes));
    }

    // Read image data
    png_read_image(png, row_pointers);

    png_read_end(png, NULL);

    return surface;
}

PNGImageReader::~PNGImageReader() {
    png_destroy_read_struct(&png, &info, NULL);
    png = NULL;
    info = NULL;
}



JPEGImageReader::JPEGImageReader(const unsigned char* src, size_t len) :
        ImageReader(src, len) {
    //  Allocate and initialize a JPEG decompression object
    //  Specify the source of the compressed data (eg, a file)
    //  Call jpeg_read_header() to obtain image info
}

unsigned char* JPEGImageReader::decode() {
    //  Set parameters for decompression
    //  jpeg_start_decompress(...);
    //  while (scan lines remain to be read)
    //      jpeg_read_scanlines(...);
    //  jpeg_finish_decompress(...);
    //  Release the JPEG decompression object
}

JPEGImageReader::~JPEGImageReader() {

}




ImageReader* ImageReader::create(const unsigned char* src, size_t len) {
    if (png_sig_cmp((png_bytep)src, 0, 8) == 0) {
        return new PNGImageReader(src, len);
    } else if (src[0] == '\xFF' && src[1] == '\xD8') {
        return new JPEGImageReader(src, len);
    }

    return NULL;
}

ImageReader::~ImageReader() {}
