#ifndef NODE_BLEND_SRC_WEBP_IO_HPP
#define NODE_BLEND_SRC_WEBP_IO_HPP

#include "webp/encode.h"

#include <ostream>

template <typename T1, typename T2>
void save_as_webp(T1& file, int quality, T2 const& image)
{
    uint8_t *output = NULL;
    int stride = sizeof(typename T2::pixel_type) * image.width();
    int width = image.width();
    int height = image.height();
    const uint8_t *bytes = image.getBytes();

    // TODO: Use advanced API to stream directly to the file stream.
    // TODO: Expose more options to JS land.
    size_t length = WebPEncodeRGBA(bytes, width, height, stride, quality, &output);

    file.write((char *)output, length);
    file.flush();

    free(output);
}

#endif // NODE_BLEND_SRC_WEBP_IO_HPP
