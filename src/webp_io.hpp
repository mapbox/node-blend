#ifndef NODE_BLEND_SRC_WEBP_IO_HPP
#define NODE_BLEND_SRC_WEBP_IO_HPP

#include "webp/encode.h"

#include <ostream>
#include <sstream>
#include <stdexcept>


template <typename T>
int webp_stream_write(const uint8_t* data, size_t data_size, const WebPPicture* picture)
{
    T* out = static_cast<T*>(picture->custom_ptr);
    out->write(reinterpret_cast<const char*>(data), data_size);
    return true;
}

std::string webp_encoding_error(WebPEncodingError error) {
    std::ostringstream os;
    switch (error) {
        case VP8_ENC_ERROR_OUT_OF_MEMORY: os << "memory error allocating objects"; break;
        case VP8_ENC_ERROR_BITSTREAM_OUT_OF_MEMORY: os << "memory error while flushing bits"; break;
        case VP8_ENC_ERROR_NULL_PARAMETER: os << "a pointer parameter is NULL"; break;
        case VP8_ENC_ERROR_INVALID_CONFIGURATION: os << "configuration is invalid"; break;
        case VP8_ENC_ERROR_BAD_DIMENSION: os << "picture has invalid width/height"; break;
        case VP8_ENC_ERROR_PARTITION0_OVERFLOW: os << "partition is bigger than 512k"; break;
        case VP8_ENC_ERROR_PARTITION_OVERFLOW: os << "partition is bigger than 16M"; break;
        case VP8_ENC_ERROR_BAD_WRITE: os << "error while flushing bytes"; break;
        case VP8_ENC_ERROR_FILE_TOO_BIG: os << "file is bigger than 4G"; break;
        default: os << "unknown error (" << error << ")"; break;
    }
    os << " during encoding";
    return os.str();
}

template <typename T1, typename T2>
void save_as_webp(T1& file, int quality, int compression, T2 const& image)
{
    WebPConfig config;
    if (!WebPConfigPreset(&config, WEBP_PRESET_DEFAULT, quality)) {
        throw new std::runtime_error("version mismatch");
    }

    // Add additional tuning
    if (compression >= 0) config.method = compression;

    bool valid = WebPValidateConfig(&config);
    if (!valid) {
        throw std::runtime_error("Invalid configuration");
    }

    WebPPicture pic;
    if (!WebPPictureInit(&pic)) {
        throw new std::runtime_error("version mismatch");
    }
    pic.width = image.width();
    pic.height = image.height();
    if (!WebPPictureAlloc(&pic)) {
        throw new std::runtime_error("memory error");
    }

    int stride = sizeof(typename T2::pixel_type) * image.width();
    const uint8_t *bytes = image.getBytes();
    int ok = WebPPictureImportRGBA(&pic, bytes, stride);

    pic.writer = webp_stream_write<T1>;
    pic.custom_ptr = &file;

    ok = WebPEncode(&config, &pic);
    WebPPictureFree(&pic);
    if (!ok) {
        throw std::runtime_error(webp_encoding_error(pic.error_code));
    }

    file.flush();
}

#endif // NODE_BLEND_SRC_WEBP_IO_HPP
