#include "writer.h"
#include "quantize.h"


/**
 * Compute a palette for the given RGBA rasterBuffer using a median cut quantization.
 * - rb: the rasterBuffer to quantize
 * - reqcolors: the desired number of colors the palette should contain. will be set
 *   with the actual number of entries in the computed palette
 * - palette: preallocated array of palette entries that will be populated by the
 *   function
 * - maxval: max value of pixel intensity. In some cases, the input data has to
 *   be rescaled to compute the quantization. if the returned value of maxscale is
 *   less than 255, this means that the input pixels have been rescaled, and that
 *   the returned palette must be upscaled before being written to the png file
 * - forced_palette: entries that should appear in the computed palette
 * - num_forced_palette_entries: number of entries contained in "force_palette". if 0,
 *   "force_palette" can be NULL
 */
int Blend_QuantizeImage(unsigned const char* source,
        unsigned long width,
        unsigned long height,
        unsigned int *reqcolors, rgbaPixel *palette,
        unsigned int *maxval,
        rgbaPixel *forced_palette, int num_forced_palette_entries) {

    rgbaPixel **apixels=NULL; /* pointer to the start rows of truecolor pixels */
    register rgbaPixel *pP;
    register int col;

    unsigned char newmaxval;
    acolorhist_vector achv, acolormap=NULL;

    int row;
    int colors;
    int newcolors = 0;

    int x;
    /*  int channels;  */


    *maxval = 255;

    apixels = (rgbaPixel**)malloc(height * sizeof(rgbaPixel**));
    if (!apixels) return 0;

    for (row = 0; row < height; row++) {
        apixels[row] = (rgbaPixel*)(source + (4 * width * row));
    }

   /*
    ** Step 2: attempt to make a histogram of the colors, unclustered.
    ** If at first we don't succeed, lower maxval to increase color
    ** coherence and try again.  This will eventually terminate, with
    ** maxval at worst 15, since 32^3 is approximately MAXCOLORS.
                  [GRR POSSIBLE BUG:  what about 32^4 ?]
    */
    for ( ; ; ) {
        achv = pam_computeacolorhist(
            apixels, width, height, MAXCOLORS, &colors );
        if ( achv != (acolorhist_vector) 0 )
            break;
        newmaxval = *maxval / 2;
        for ( row = 0; row < height; ++row )
            for ( col = 0, pP = apixels[row]; col < width; ++col, ++pP )
                PAM_DEPTH( *pP, *pP, *maxval, newmaxval );
        *maxval = newmaxval;
    }
    newcolors = MIN(colors, *reqcolors);
    acolormap = mediancut(achv, colors, width * height, *maxval, newcolors);
    pam_freeacolorhist(achv);

    *reqcolors = newcolors;

    for (x = 0; x < newcolors; ++x) {
        palette[x].r = acolormap[x].acolor.r;
        palette[x].g = acolormap[x].acolor.g;
        palette[x].b = acolormap[x].acolor.b;
        palette[x].a = acolormap[x].acolor.a;
    }

    free(acolormap);
    free(apixels);
    return 1;
}

int Blend_RemapPalette(unsigned char *pixels, int npixels,
      rgbaPixel *palette, int numPaletteEntries, unsigned int maxval,
      rgbPixel *rgb, unsigned char *a, int *num_a) {
   int bot_idx, top_idx, x;
   int remap[256];
   /*
    ** remap the palette colors so that all entries with
    ** the maximal alpha value (i.e., fully opaque) are at the end and can
    ** therefore be omitted from the tRNS chunk.  Note that the ordering of
    ** opaque entries is reversed from how Step 3 arranged them--not that
    ** this should matter to anyone.
    */

   for (top_idx = numPaletteEntries-1, bot_idx = x = 0;  x < numPaletteEntries;  ++x) {
      if (palette[x].a == maxval)
         remap[x] = top_idx--;
      else
         remap[x] = bot_idx++;
   }
   /* sanity check:  top and bottom indices should have just crossed paths */
   if (bot_idx != top_idx + 1) {
      return 0;
   }

   *num_a = bot_idx;

   for(x=0;x<npixels;x++)
      pixels[x] = remap[pixels[x]];

   for (x = 0; x < numPaletteEntries; ++x) {
      if(maxval == 255) {
         a[remap[x]] = palette[x].a;
         rgb[remap[x]].r = palette[x].r;
         rgb[remap[x]].g = palette[x].g;
         rgb[remap[x]].b = palette[x].b;
      } else {
         rgb[remap[x]].r = (palette[x].r * 255 + (maxval >> 1)) / maxval;
         rgb[remap[x]].g = (palette[x].g * 255 + (maxval >> 1)) / maxval;
         rgb[remap[x]].b = (palette[x].b * 255 + (maxval >> 1)) / maxval;
         a[remap[x]] = (palette[x].a * 255 + (maxval >> 1)) / maxval;
      }
   }
   return 1;
}

int Blend_ClassifyImage(unsigned const char* source,
        unsigned long width,
        unsigned long height,
        unsigned char *pixels,
        rgbaPixel *palette,
        int numPaletteEntries) {
   register int ind;
   unsigned char *outrow,*pQ;
   register rgbaPixel *pP;
   acolorhash_table acht;
   int usehash, row, col;
   /*
    ** Step 4: map the colors in the image to their closest match in the
    ** new colormap, and write 'em out.
    */
   acht = pam_allocacolorhash( );
   usehash = 1;

   for ( row = 0; row < height; ++row ) {
      outrow = &(pixels[row*width]);
      col = 0;
      pP = (rgbaPixel*)(source + (4 * width * row));
      pQ = outrow;
      do {
         /* Check hash table to see if we have already matched this color. */
         ind = pam_lookupacolor( acht, pP );
         if ( ind == -1 ) {
            /* No; search acolormap for closest match. */
            register int i, r1, g1, b1, a1, r2, g2, b2, a2;
            register long dist, newdist;

            r1 = PAM_GETR( *pP );
            g1 = PAM_GETG( *pP );
            b1 = PAM_GETB( *pP );
            a1 = PAM_GETA( *pP );
            dist = 2000000000;
            for ( i = 0; i < numPaletteEntries; ++i ) {
               r2 = PAM_GETR( palette[i] );
               g2 = PAM_GETG( palette[i] );
               b2 = PAM_GETB( palette[i] );
               a2 = PAM_GETA( palette[i] );
               /* GRR POSSIBLE BUG */
               newdist = ( r1 - r2 ) * ( r1 - r2 ) +  /* may overflow? */
                     ( g1 - g2 ) * ( g1 - g2 ) +
                     ( b1 - b2 ) * ( b1 - b2 ) +
                     ( a1 - a2 ) * ( a1 - a2 );
               if ( newdist < dist ) {
                  ind = i;
                  dist = newdist;
               }
            }
            if ( usehash ) {
               if ( pam_addtoacolorhash( acht, pP, ind ) < 0 ) {
                  usehash = 0;
               }
            }
         }

         /*          *pP = acolormap[ind].acolor;  */
         *pQ = (unsigned char)ind;

         ++col;
         ++pP;
         ++pQ;

      }
      while ( col != width );
   }
   pam_freeacolorhash(acht);

   return 1;
}



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


void Blend_EncodePNG8Bit(unsigned const char* source, BlendBaton* baton,
        unsigned long width, unsigned long height) {
    unsigned int numPaletteEntries = baton->quality;
    unsigned char *pixels = (unsigned char*)malloc(width * height * sizeof(unsigned char));
    rgbaPixel palette[256];
    unsigned int maxval;
    Blend_QuantizeImage(source, width, height, &numPaletteEntries, palette, &maxval, NULL, 0);
    Blend_ClassifyImage(source, width, height, pixels, palette, numPaletteEntries);
    rgbPixel rgb[256];
    unsigned char a[256];
    int num_a;
    int row, sample_depth;


    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info_ptr = png_create_info_struct(png_ptr);

    png_set_compression_level(png_ptr, Z_BEST_SPEED);
    png_set_compression_buffer_size(png_ptr, 32768);

    if (numPaletteEntries <= 2)
        sample_depth = 1;
    else if (numPaletteEntries <= 4)
        sample_depth = 2;
    else if (numPaletteEntries <= 16)
        sample_depth = 4;
    else
        sample_depth = 8;

    png_set_IHDR(png_ptr, info_ptr, width, height, sample_depth,
                 PNG_COLOR_TYPE_PALETTE,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);

    Blend_RemapPalette(pixels, width * height, palette, numPaletteEntries,
                 maxval, rgb, a, &num_a);

    png_set_PLTE(png_ptr, info_ptr, (png_colorp)(rgb), numPaletteEntries);
    if (num_a) {
        png_set_tRNS(png_ptr, info_ptr, a, num_a, NULL);
    }

    png_set_write_fn(png_ptr, (png_voidp)baton, Blend_WritePNG, NULL);
    png_set_filter(png_ptr, PNG_FILTER_TYPE_BASE, PNG_FILTER_NONE);

    png_write_info(png_ptr, info_ptr);
    png_set_packing(png_ptr);

    for (row=0; row < height; row++) {
        unsigned char *rowptr = &(pixels[row * width]);
        png_write_row(png_ptr, rowptr);
    }
    png_write_end(png_ptr, info_ptr);

    png_destroy_write_struct(&png_ptr, &info_ptr);
    free(pixels);
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
    } else if (baton->format == BLEND_FORMAT_PNG && baton->quality > 0) {
        Blend_EncodePNG8Bit(source, baton, width, height);
    } else {
        Blend_EncodePNG(source, baton, width, height, alpha);
    }
}
