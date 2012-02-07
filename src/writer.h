#ifndef NODE_BLEND_SRC_WRITER_H
#define NODE_BLEND_SRC_WRITER_H

#include <png.h>
#include <jpeglib.h>
#include <zlib.h>
#include <assert.h>

#include <cstdlib>
#include <cstring>

#include <string>

#include "blend.h"

struct JPEGErrorManager {
    jpeg_error_mgr pub;
    BlendBaton* baton;
};

void Blend_WritePNG(png_structp png_ptr, png_bytep data, png_size_t length);
void Blend_EncodePNG(unsigned const char* source, BlendBaton* baton,
                     unsigned long width, unsigned long height, bool alpha);

void Blend_EncodeJPEG(unsigned const char* source, BlendBaton* baton,
                      unsigned long width, unsigned long height, bool alpha);

void Blend_Encode(unsigned const char* source, BlendBaton* baton,
                  unsigned long width, unsigned long height, bool alpha);


#endif
