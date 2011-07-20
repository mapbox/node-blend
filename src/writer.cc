#include "writer.h"
#include "octree.h"


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



#define MAX_OCTREE_LEVELS 4

void Blend_ReduceColors(unsigned const char* source, unsigned long width,
                        unsigned long height, unsigned char* out,
                        octree<rgb> trees[], unsigned limits[], unsigned levels,
                        std::vector<unsigned> & alpha) {
    std::vector<unsigned> alphaCount(alpha.size());
    for (unsigned i = 0; i < alpha.size(); i++) {
        alpha[i] = 0;
        alphaCount[i] = 0;
    }

    for (unsigned y = 0; y < height; ++y) {
        unsigned const char* row = &(source[y * 4 * width]);
        unsigned char* row_out = &(out[y * width]);
        for (unsigned x = 0; x < width; ++x) {
            unsigned const char* val = &row[x * 4];
            rgb c(val[0], val[1], val[2]);
            byte index = 0;
            int idx = -1;
            for (int j = levels - 1; j > 0; j--) {
                if (val[3] >= limits[j] && trees[j].colors() > 0) {
                    index = idx = trees[j].quantize(c);
                    break;
                }
            }
            if (idx >= 0 && idx < (int)alpha.size()) {
                alpha[idx] += val[3];
                alphaCount[idx]++;
            }

            row_out[x] = index;
        }
    }
    for (unsigned i = 0; i < alpha.size(); i++) {
        if (alphaCount[i] != 0) {
            alpha[i] /= alphaCount[i];
        }
    }
}


void Blend_EncodePNG(unsigned const char* image, BlendBaton* baton,
                     unsigned long width, unsigned long height,
                     unsigned color_depth, std::vector<rgb> & palette,
                     std::vector<unsigned> &alpha) {
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info_ptr = png_create_info_struct(png_ptr);

    png_set_compression_level(png_ptr, Z_BEST_SPEED);
    png_set_compression_buffer_size(png_ptr, 32768);

    png_set_IHDR(png_ptr, info_ptr, width, height, color_depth,
                 PNG_COLOR_TYPE_PALETTE,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);

    png_set_PLTE(png_ptr, info_ptr, reinterpret_cast<png_color*>(&palette[0]), palette.size());

    // make transparent lowest indexes, so tRNS is small
    if (alpha.size() > 0) {
        std::vector<png_byte> trans(alpha.size());
        unsigned alphaSize = 0; //truncate to nonopaque values
        for (unsigned i = 0; i < alpha.size(); i++) {
            trans[i] = alpha[i];
            if (alpha[i] < 255) {
                alphaSize = i + 1;
            }
        }
        if (alphaSize > 0) {
            png_set_tRNS(png_ptr, info_ptr, (png_bytep)&trans[0], alphaSize, 0);
        }
    }

    png_set_write_fn(png_ptr, (png_voidp)baton, Blend_WritePNG, NULL);
    png_set_filter(png_ptr, PNG_FILTER_TYPE_BASE, PNG_FILTER_NONE);

    png_write_info(png_ptr, info_ptr);
    png_set_packing(png_ptr);

    for (unsigned int y = 0; y < height; y++) {
        png_write_row(png_ptr, (png_bytep)(image + (width * y)));
    }
    png_write_end(png_ptr, info_ptr);

    png_destroy_write_struct(&png_ptr, &info_ptr);
}

void Blend_EncodePNGOctree(unsigned const char* source, BlendBaton* baton,
                           unsigned long width, unsigned long height,
                           bool alpha) {
    unsigned max_colors = baton->quality;
    int trans_mode = alpha ? 4 : 0;

    // number of alpha ranges in png256 format; 2 results in smallest image with binary transparency
    // 3 is minimum for semitransparency, 4 is recommended, anything else is worse
    const unsigned TRANSPARENCY_LEVELS = (trans_mode == 2 || trans_mode < 0) ? MAX_OCTREE_LEVELS : 2;
    unsigned alphaHist[256];//transparency histogram
    unsigned semiCount = 0;//sum of semitransparent pixels
    unsigned meanAlpha = 0;
    for (int i = 0; i < 256; i++) {
        alphaHist[i] = 0;
    }
    for (unsigned y = 0; y < height; ++y) {
        for (unsigned x = 0; x < width; ++x) {
            unsigned val = source[y * 4 * width + 4 * x + 3]; // Get alpha value of that pixel.
            if (trans_mode == 0) {
                val = 255;
            }
            alphaHist[val]++;
            meanAlpha += val;
            if (val > 0 && val < 255) {
                semiCount++;
            }
        }
    }
    meanAlpha /= width * height;

    // transparency ranges division points
    unsigned limits[MAX_OCTREE_LEVELS + 1];
    limits[0] = 0;
    limits[1] = (alphaHist[0] > 0) ? 1 : 0;
    limits[TRANSPARENCY_LEVELS] = 256;
    unsigned alphaHistSum = 0;
    for (unsigned j = 1; j < TRANSPARENCY_LEVELS; j++) {
        limits[j] = limits[1];
    }
    for (unsigned i = 1; i < 256; i++) {
        alphaHistSum += alphaHist[i];
        for (unsigned j = 1; j < TRANSPARENCY_LEVELS; j++) {
            if (alphaHistSum < semiCount * (j) / 4) {
                limits[j] = i;
            }
        }
    }
    // avoid too wide full transparent range
    if (limits[1] > 256 / (TRANSPARENCY_LEVELS - 1)) {
        limits[1] = 256 / (TRANSPARENCY_LEVELS - 1);
    }

    // avoid too wide full opaque range
    if (limits[TRANSPARENCY_LEVELS - 1] < 212) {
        limits[TRANSPARENCY_LEVELS - 1] = 212;
    }

    if (TRANSPARENCY_LEVELS == 2) {
        limits[1] = 127;
    }

    // estimated number of colors from palette assigned to chosen ranges
    unsigned cols[MAX_OCTREE_LEVELS];

    // count colors
    for (unsigned j = 1; j <= TRANSPARENCY_LEVELS; j++) {
        cols[j - 1] = 0;
        for (unsigned i = limits[j - 1]; i < limits[j]; i++) {
            cols[j - 1] += alphaHist[i];
        }
    }

    unsigned divCoef = width * height - cols[0];
    if (divCoef == 0) {
        divCoef = 1;
    }
    cols[0] = cols[0] > 0 ? 1 : 0; // fully transparent color (one or not at all)

    if (max_colors >= 64) {
        // give chance less populated but not empty cols to have at least few colors(12)
        unsigned minCols = (12 + 1) * divCoef / (max_colors - cols[0]);
        for (unsigned j = 1; j < TRANSPARENCY_LEVELS; j++) {
            if (cols[j] > 12 && cols[j] < minCols) {
                divCoef += minCols - cols[j];
                cols[j] = minCols;
            }
        }
    }
    unsigned usedColors = cols[0];
    for (unsigned j = 1; j < TRANSPARENCY_LEVELS - 1; j++) {
        cols[j] = cols[j] * (max_colors - cols[0]) / divCoef;
        usedColors += cols[j];
    }
    // use rest for most opaque group of pixels
    cols[TRANSPARENCY_LEVELS - 1] = max_colors - usedColors;

    // no transparency
    if (trans_mode == 0) {
        limits[1] = 0;
        cols[0] = 0;
        cols[1] = max_colors;
    }

    // octree table for separate alpha range with 1-based index (0 is fully transparent: no color)
    octree<rgb> trees[MAX_OCTREE_LEVELS];
    for (unsigned j = 1; j < TRANSPARENCY_LEVELS; j++) {
        trees[j].setMaxColors(cols[j]);
    }

    for (unsigned y = 0; y < height; ++y) {
        unsigned const char* row = &(source[y * 4 * width]);
        for (unsigned x = 0; x < width; ++x) {
            unsigned const char* val = &(row[x * 4]);

            // insert to proper tree based on alpha range
            for (unsigned j = TRANSPARENCY_LEVELS - 1; j > 0; j--) {
                if (cols[j] > 0 && val[3] >= limits[j]) {
                    trees[j].insert(rgb(val[0], val[1], val[2]));
                    break;
                }
            }
        }
    }
    unsigned leftovers = 0;
    std::vector<rgb> palette;
    if (cols[0]) {
        palette.push_back(rgb(0, 0, 0));
    }

    for (unsigned j = 1; j < TRANSPARENCY_LEVELS; j++) {
        if (cols[j] > 0) {
            if (leftovers > 0) {
                cols[j] += leftovers;
                trees[j].setMaxColors(cols[j]);
                leftovers = 0;
            }
            std::vector<rgb> pal;
            trees[j].setOffset(palette.size());
            trees[j].create_palette(pal);
            assert(pal.size() <= max_colors);
            leftovers = cols[j] - pal.size();
            cols[j] = pal.size();
            for (unsigned i = 0; i < pal.size(); i++) {
                palette.push_back(pal[i]);
            }
            assert(palette.size() <= 256);
        }
    }

    //transparency values per palette index
    std::vector<unsigned> alphaTable;
    //alphaTable.resize(palette.size());//allow semitransparency also in almost opaque range
    if (trans_mode != 0) {
        alphaTable.resize(palette.size() - cols[TRANSPARENCY_LEVELS - 1]);
    }


    unsigned char* reduced_image = (unsigned char*)malloc(width * height);

    int depth = 1;
    if (palette.size() > 16) {
        depth = 8;
    } else if (palette.size() > 4) {
        depth = 4;
    } else if (palette.size() > 2) {
        depth = 2;
    }

    if (palette.size() == 1) {
        // 1 color image ->  write 1-bit color depth PNG
        memset(reduced_image, 0, width * height);
        if (meanAlpha < 255 && cols[0] == 0) {
            alphaTable.resize(1);
            alphaTable[0] = meanAlpha;
        }
    } else {
        Blend_ReduceColors(source, width, height, reduced_image, trees, limits, TRANSPARENCY_LEVELS, alphaTable);
    }

    Blend_EncodePNG(reduced_image, baton, width, height, depth, palette, alphaTable);
    free(reduced_image);
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
    (*cinfo->err->format_message)(cinfo, buffer);
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
    } else if (baton->format == BLEND_FORMAT_PNG && baton->quality > 0) {
        Blend_EncodePNGOctree(source, baton, width, height, alpha);
    } else {
        Blend_EncodePNG(source, baton, width, height, alpha);
    }
}
