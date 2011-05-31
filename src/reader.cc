#include "reader.h"

PNGImageReader::PNGImageReader(unsigned char* src, size_t len) :
        ImageReader(src, len), depth(0), color(-1) {
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

    png_destroy_read_struct(&png, &info, NULL);
    png = NULL;
    info = NULL;

    return surface;
}



JPEGImageReader::JPEGImageReader(unsigned char* src, size_t len) :
        ImageReader(src, len) {
    info.err = jpeg_std_error(&err);
    jpeg_create_decompress(&info);
    jpeg_mem_src(&info, src, len);
    jpeg_read_header(&info, TRUE);
    width = info.image_width;
    height = info.image_height;
    alpha = false;
    assert(info.data_precision == 8);
    assert(info.num_components == 3);
}

unsigned char* JPEGImageReader::decode() {
    size_t length = width * height * 4;
    size_t offset = 0;
    unsigned char* surface = (unsigned char*)malloc(length);
    assert(surface);

    jpeg_start_decompress(&info);
    while (info.output_scanline < info.output_height) {
        assert(offset < length);
        unsigned char* destination = surface + offset;
        jpeg_read_scanlines(&info, &destination, width * 3);

        // Convert from RGB to RGBA
        unsigned int* dest = (unsigned int*)destination;
        for (int i = width - 1, pos = i * 3; i >= 0; pos = --i * 3) {
            // Read 3 bytes and write it to 4 bytes.
            dest[i] = 0xFF << 24 |
                      destination[pos] |
                      destination[pos + 1] << 8 |
                      destination[pos + 2] << 16;
        }

        offset += width * 4;
    }

    jpeg_finish_decompress(&info);
    jpeg_destroy_decompress(&info);

    return surface;
}



ImageReader* ImageReader::create(unsigned char* src, size_t len) {
    if (png_sig_cmp((png_bytep)src, 0, 8) == 0) {
        return new PNGImageReader(src, len);
    } else if (src[0] == 255 && src[1] == 216) {
        return new JPEGImageReader(src, len);
    }

    return NULL;
}
