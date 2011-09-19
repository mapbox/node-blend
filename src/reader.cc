#include "reader.h"

PNGImageReader::PNGImageReader(unsigned char* src, size_t len) :
    ImageReader(src, len), depth(0), color(-1) {
    // Decode PNG header.
    png = png_create_read_struct(PNG_LIBPNG_VER_STRING, (png_voidp)this, errorHandler, warningHandler);
    assert(png);
    info = png_create_info_struct(png);
    assert(info);

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, NULL);
        width = 0;
        height = 0;
        return;
    }

    png_set_read_fn(png, this, readCallback);
    png_read_info(png, info);
    png_get_IHDR(png, info, &width, &height, &depth, &color, NULL, NULL, NULL);
    alpha = (color & PNG_COLOR_MASK_ALPHA) || png_get_valid(png, info, PNG_INFO_tRNS);
}

void PNGImageReader::readCallback(png_structp png, png_bytep data, png_size_t length) {
    PNGImageReader* reader = static_cast<PNGImageReader*>(png_get_error_ptr(png));

    // Read `length` bytes into `data`.
    if (reader->pos + length > reader->length) {
        png_error(png, "Read Error");
        return;
    }

    memcpy(data, reader->source + reader->pos, length);
    reader->pos += length;
}

void PNGImageReader::errorHandler(png_structp png, png_const_charp error_msg) {
    PNGImageReader* reader = static_cast<PNGImageReader*>(png_get_io_ptr(png));

    reader->message = error_msg;

    if (png) {
        longjmp(png_jmpbuf(png), 1);
    }
    exit(1);
}

void PNGImageReader::warningHandler(png_structp png, png_const_charp error_msg) {
    PNGImageReader* reader = static_cast<PNGImageReader*>(png_get_io_ptr(png));
    reader->warnings.push_back(error_msg);
}

unsigned char* PNGImageReader::decode() {
    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, NULL);
        width = 0;
        height = 0;
        return NULL;
    }

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

JPEGImageReader::JPEGImageReader(unsigned char* src, size_t len) :
    ImageReader(src, len) {
    err.reader = this;
    info.err = jpeg_std_error(&err.pub);
    err.pub.error_exit = errorHandler;
    err.pub.output_message = errorMessage;

    if (setjmp(err.jump)) {
        jpeg_destroy_decompress(&info);
        width = 0;
        height = 0;

        // Error message was set by JPEGImageReader::errorMessage.
        return;
    }
    jpeg_create_decompress(&info);
    jpeg_mem_src(&info, src, len);
    jpeg_read_header(&info, TRUE);
    width = info.image_width;
    height = info.image_height;
    alpha = false;
}

void JPEGImageReader::errorHandler(j_common_ptr cinfo) {
    // libjpeg recommends doing this memory alignment trickery.
    JPEGErrorManager* error = (JPEGErrorManager*)cinfo->err;

    (*error->pub.output_message)(cinfo);

    jpeg_destroy(cinfo);

    /* Return control to the setjmp point */
    longjmp(error->jump, 1);
}

void JPEGImageReader::errorMessage(j_common_ptr cinfo) {
    // libjpeg recommends doing this memory alignment trickery.
    JPEGErrorManager* error = (JPEGErrorManager*)cinfo->err;

    char buffer[JMSG_LENGTH_MAX];
    (*cinfo->err->format_message)(cinfo, buffer);
    error->reader->message = buffer;
}

unsigned char* JPEGImageReader::decode() {
    if (info.data_precision != 8) return NULL;
    if (info.num_components != 3 && info.num_components != 1) return NULL;

    size_t length = width * height * 4;
    unsigned char* surface = (unsigned char*)malloc(length);
    if (surface == NULL) {
        message = "Insufficient memory";
        jpeg_destroy_decompress(&info);
        return NULL;
    }

    if (setjmp(err.jump)) {
        jpeg_destroy_decompress(&info);
        free(surface);

        // Error message was set by JPEGImageReader::errorMessage.
        return NULL;
    }

    info.out_color_space = JCS_RGB;

    jpeg_start_decompress(&info);

    unsigned char* row_pointers[height];
    for (unsigned i = 0; i < height; i++) {
        row_pointers[i] = (unsigned char*)(surface + (i * width * 4));
    }

    size_t offset = 0;
    while (info.output_scanline < info.output_height) {
        offset += jpeg_read_scanlines(&info, row_pointers + offset, height - offset);
    }

    // Convert to RGBA.
    for (unsigned i = 0; i < height; i++) {
        unsigned char* destination = surface + i * width * 4;
        unsigned int* image = (unsigned int*)destination;
        for (int j = width - 1, k = j * 3; j >= 0; k = --j * 3) {
            image[j] = 0xFF << 24 | destination[k + 2] << 16 |
                       destination[k + 1] << 8 | destination[k];
        }
    }

    jpeg_finish_decompress(&info);

    return surface;
}


JPEGImageReader::~JPEGImageReader() {
    jpeg_destroy_decompress(&info);
}


ImageReader* ImageReader::create(unsigned char* src, size_t len) {
    if (png_sig_cmp((png_bytep)src, 0, 8) == 0) {
        return new PNGImageReader(src, len);
    } else if (src[0] == 255 && src[1] == 216) {
        return new JPEGImageReader(src, len);
    } else {
        return new ImageReader("Unknown image format");
    }
}
