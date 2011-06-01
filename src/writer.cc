#include "writer.h"

void Blend_WritePNG(png_structp png_ptr, png_bytep data, png_size_t length) {
    BlendBaton* baton = (BlendBaton*)png_get_io_ptr(png_ptr);

    if (baton->result == NULL || baton->max < baton->length + length) {
        int increase = baton->length ? 4 * length : 32768;
        baton->result = (unsigned char*)realloc(baton->result, baton->max + increase);
        baton->max += increase;
    }

    // TODO: implement OOM check
    assert(baton->result);

    memcpy(baton->result + baton->length, data, length);
    baton->length += length;
}

void Blend_EncodePNG(unsigned const char* source, BlendBaton* baton,
        unsigned long width, unsigned long height, bool alpha) {
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info_ptr = png_create_info_struct(png_ptr);

    png_set_compression_level(png_ptr, Z_BEST_SPEED);
    png_set_compression_buffer_size(png_ptr, 32768);

    png_set_IHDR(png_ptr, info_ptr, width, height, 8,
                 alpha ? PNG_COLOR_TYPE_RGB_ALPHA : PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);

    png_bytep row_pointers[height];
    for (unsigned i = 0; i < height; i++) {
        row_pointers[i] = (png_bytep)(source + (4 * width * i));
    }
    png_set_rows(png_ptr, info_ptr, (png_bytepp)&row_pointers);
    png_set_filter(png_ptr, PNG_FILTER_TYPE_BASE, PNG_FILTER_NONE);

    png_set_write_fn(png_ptr, (png_voidp)baton, Blend_WritePNG, NULL);
    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_STRIP_FILLER_AFTER, NULL);
    png_destroy_write_struct(&png_ptr, &info_ptr);
}


void JPEG_errorHandler(j_common_ptr cinfo) {
    // libjpeg recommends doing this memory alignment trickery.
    JPEGErrorManager* err = (JPEGErrorManager*)cinfo->err;

    /* Return control to the setjmp point */
    longjmp(err->jump, 1);
}

void JPEG_errorMessage(j_common_ptr cinfo) {
    // libjpeg recommends doing this memory alignment trickery.
    JPEGErrorManager* err = (JPEGErrorManager*)cinfo->err;

    char buffer[JMSG_LENGTH_MAX];
    (*cinfo->err->format_message) (cinfo, buffer);
    err->baton->message = buffer;
}

void Blend_EncodeJPEG(unsigned const char* source, BlendBaton* baton,
        unsigned long width, unsigned long height, bool alpha) {
    jpeg_compress_struct info;
    JPEGErrorManager err;

    info.err = jpeg_std_error(&err.pub);
    err.pub.error_exit = JPEG_errorHandler;
    err.pub.output_message = JPEG_errorMessage;

    if (setjmp(err.jump)) {
        // Error message was set by JPEG_errorMessage.
        baton->error = true;
        if (baton->result) {
            free(baton->result);
            baton->result = NULL;
        }
        return;
    }

    jpeg_create_compress(&info);

    unsigned char* result = NULL;
    unsigned long length = 0;
    jpeg_mem_dest(&info, &result, &length);

    info.image_width = width;
    info.image_height = height;
    info.input_components = 3;
    info.in_color_space = JCS_RGB;
    jpeg_set_defaults(&info);
    jpeg_set_quality(&info, baton->quality, TRUE);

    jpeg_start_compress(&info, TRUE);

    unsigned char* row = (unsigned char*)malloc(width * 3);
    while (info.next_scanline < info.image_height) {
        // Get rid of the alpha channel.
        const unsigned char* scanline = source + info.next_scanline * width * 4;
        for (int i = 0, j = 0, end = width * 3; i < end; j++) {
            row[i++] = scanline[j++];
            row[i++] = scanline[j++];
            row[i++] = scanline[j++];
        }

        jpeg_write_scanlines(&info, &row, 1);
    }

    free(row);

    jpeg_finish_compress(&info);
    jpeg_destroy_compress(&info);

    baton->result = result;
    baton->length = length;
}

void Blend_Encode(unsigned const char* source, BlendBaton* baton,
        unsigned long width, unsigned long height, bool alpha) {
    if (baton->format == BLEND_FORMAT_JPEG) {
        Blend_EncodeJPEG(source, baton, width, height, alpha);
    } else {
        Blend_EncodePNG(source, baton, width, height, alpha);
    }
}
