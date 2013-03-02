#include "reader.hpp"
#include <webp/decode.h>

#include <exception>

PNGImageReader::PNGImageReader(unsigned char* src, size_t len) :
    ImageReader(src, len), depth(0), color(-1), interlace(PNG_INTERLACE_NONE) {
    // Decode PNG header.
    png = png_create_read_struct(PNG_LIBPNG_VER_STRING, (png_voidp)this, errorHandler, warningHandler);
    assert(png);
    info = png_create_info_struct(png);
    assert(info);

    try {
        png_set_read_fn(png, this, readCallback);
        png_read_info(png, info);
        png_get_IHDR(png, info, &width, &height, &depth, &color, &interlace, NULL, NULL);
        alpha = (color & PNG_COLOR_MASK_ALPHA) || png_get_valid(png, info, PNG_INFO_tRNS);
    } catch(std::exception& e) {
        png_destroy_read_struct(&png, &info, NULL);
        width = 0;
        height = 0;
    }
}

void PNGImageReader::readCallback(png_structp png, png_bytep data, png_size_t length) {
    PNGImageReader* reader = static_cast<PNGImageReader*>(png_get_error_ptr(png));

    // Read `length` bytes into `data`.
    if (reader->pos + length > reader->length) {
        png_error(png, "Read Error");
    } else {
        memcpy(data, reader->source + reader->pos, length);
        reader->pos += length;
    }
}

void PNGImageReader::errorHandler(png_structp png, png_const_charp error_msg) {
    PNGImageReader* reader = static_cast<PNGImageReader*>(png_get_io_ptr(png));
    reader->message = error_msg;
    throw std::exception();
}

void PNGImageReader::warningHandler(png_structp png, png_const_charp error_msg) {
    PNGImageReader* reader = static_cast<PNGImageReader*>(png_get_io_ptr(png));
    reader->warnings.push_back(error_msg);
}

bool PNGImageReader::decode() {
    try {
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

        if (interlace == PNG_INTERLACE_ADAM7)
            png_set_interlace_handling(png);

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

        surface = (unsigned int*)malloc(width * height * 4);
        assert(surface);

        png_bytep row_pointers[height];
        for (unsigned i = 0; i < height; i++) {
            row_pointers[i] = (unsigned char *)surface + (i * rowbytes);
        }

        // Read image data
        png_read_image(png, row_pointers);

        png_read_end(png, NULL);

        return true;
    } catch (std::exception& e) {
        png_destroy_read_struct(&png, &info, NULL);
        width = 0;
        height = 0;
        if (surface) free(surface);
        surface = NULL;

        return false;
    }
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

    try {
        jpeg_create_decompress(&info);
        jpeg_mem_src(&info, src, len);
        jpeg_read_header(&info, TRUE);
        width = info.image_width;
        height = info.image_height;
        alpha = false;
    } catch(std::exception& e) {
        jpeg_destroy_decompress(&info);
        width = 0;
        height = 0;
    }
}

void JPEGImageReader::errorHandler(j_common_ptr cinfo) {
    // libjpeg recommends doing this memory alignment trickery.
    JPEGErrorManager* error = (JPEGErrorManager*)cinfo->err;
    (*error->pub.output_message)(cinfo);
    jpeg_destroy(cinfo);
    throw std::exception();
}

void JPEGImageReader::errorMessage(j_common_ptr cinfo) {
    // libjpeg recommends doing this memory alignment trickery.
    JPEGErrorManager* error = (JPEGErrorManager*)cinfo->err;

    char buffer[JMSG_LENGTH_MAX];
    (*cinfo->err->format_message)(cinfo, buffer);
    error->reader->message = buffer;
}

bool JPEGImageReader::decode() {
    if (info.data_precision != 8) return false;
    if (info.num_components != 3 && info.num_components != 1) return false;

    size_t length = width * height * 4;
    surface = (unsigned int*)malloc(length);
    if (surface == NULL) {
        message = "Insufficient memory";
        jpeg_destroy_decompress(&info);
        return false;
    }

    try {
        info.out_color_space = JCS_RGB;

        jpeg_start_decompress(&info);

        unsigned char* row_pointers[height];
        for (unsigned i = 0; i < height; i++) {
            row_pointers[i] = (unsigned char*)surface + (i * width * 4);
        }

        size_t offset = 0;
        while (info.output_scanline < info.output_height) {
            offset += jpeg_read_scanlines(&info, row_pointers + offset, height - offset);
        }

        // Convert to RGBA.
        for (unsigned i = 0; i < height; i++) {
            unsigned char* destination = (unsigned char*)surface + i * width * 4;
            unsigned int* image = (unsigned int*)destination;
            for (int j = width - 1, k = j * 3; j >= 0; k = --j * 3) {
                image[j] = 0xFF << 24 | destination[k + 2] << 16 |
                           destination[k + 1] << 8 | destination[k];
            }
        }

        jpeg_finish_decompress(&info);

        return true;
    } catch (std::exception& e) {
        jpeg_destroy_decompress(&info);
        if (surface) free(surface);
        surface = NULL;
        return false;
    }
}


JPEGImageReader::~JPEGImageReader() {
    jpeg_destroy_decompress(&info);
}


WebPImageReader::WebPImageReader(unsigned char* src, size_t len) :
    ImageReader(src, len) {
    WebPDecoderConfig config;
    if (WebPGetFeatures(source, length, &config.input) == VP8_STATUS_OK) {
        alpha = config.input.has_alpha;
        width = config.input.width;
        height = config.input.height;
    }
}

bool WebPImageReader::decode() {
    surface = (unsigned int *)WebPDecodeRGBA(source, length, NULL, NULL);
    return surface != NULL;
}

ImageReader* ImageReader::create(unsigned char* src, size_t len) {
    if (png_sig_cmp((png_bytep)src, 0, 8) == 0) {
        return new PNGImageReader(src, len);
    } else if (len >= 2 && src[0] == 255 && src[1] == 216) {
        return new JPEGImageReader(src, len);
    } else if (len >= 12 &&
               src[0] == 'R' && src[1] == 'I' && src[2] == 'F' && src[3] == 'F' &&
               src[8] == 'W' && src[9] == 'E' && src[10] == 'B' && src[11] == 'P') {
        return new WebPImageReader(src, len);
    } else {
        return new ImageReader("Unknown image format");
    }
}
