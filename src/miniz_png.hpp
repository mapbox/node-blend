#ifndef NODE_BLEND_SRC_MINIZ_PNG_HPP
#define NODE_BLEND_SRC_MINIZ_PNG_HPP

// blend
#include "image_data.hpp"
#include "palette.hpp"

// stl
#include <vector>
#include <iostream>
#include <stdexcept>

// miniz
#define MINIZ_NO_ARCHIVE_APIS
#define MINIZ_NO_STDIO
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#include "miniz.c"

namespace MiniZ {

class PNGWriter {

public:
    PNGWriter(int level = MZ_DEFAULT_COMPRESSION) {
        if (level < 0 || level > 10) {
            throw new std::runtime_error("compression level must be between 0 and 10");
        }
        flags = s_tdefl_num_probes[level] | TDEFL_GREEDY_PARSING_FLAG | TDEFL_WRITE_ZLIB_HEADER;

        buffer.m_capacity = 8192;
        buffer.m_expandable = MZ_TRUE;
        buffer.m_pBuf = (mz_uint8 *)MZ_MALLOC(buffer.m_capacity);
        if (buffer.m_pBuf == NULL) throw new std::bad_alloc();

        compressor = (tdefl_compressor *)MZ_MALLOC(sizeof(tdefl_compressor));
        if (compressor == NULL) throw new std::bad_alloc();
    }

    ~PNGWriter() {
        MZ_FREE(compressor);
        MZ_FREE(buffer.m_pBuf);
    }

private:
    void initialize() {
        // Reset output buffer.
        buffer.m_size = 0;
        tdefl_init(compressor, tdefl_output_buffer_putter, &buffer, flags);

        // Write preamble.
        mz_bool status = tdefl_output_buffer_putter(preamble, 8, &buffer);
        if (status != MZ_TRUE) throw new std::bad_alloc();
    }

    inline void writeUInt32BE(mz_uint8 *target, mz_uint32 const& value) {
        target[0] = (value >> 24) & 0xFF;
        target[1] = (value >> 16) & 0xFF;
        target[2] = (value >> 8) & 0xFF;
        target[3] = value & 0xFF;
    }

    size_t startChunk(const mz_uint8 header[], size_t length) {
        size_t start = buffer.m_size;
        mz_bool status = tdefl_output_buffer_putter(header, length, &buffer);
        if (status != MZ_TRUE) throw new std::bad_alloc();
        return start;
    }

    void finishChunk(size_t start) {
        // Write chunk length at the beginning of the chunk.
        size_t payloadLength = buffer.m_size - start - 4 - 4;
        writeUInt32BE(buffer.m_pBuf + start, payloadLength);

        // Write CRC32 checksum. Don't include the 4-byte length, but /do/ include
        // the 4-byte chunk name.
        mz_uint32 crc = mz_crc32(MZ_CRC32_INIT, buffer.m_pBuf + start + 4, payloadLength + 4);
        mz_uint8 checksum[] = { crc >> 24, crc >> 16, crc >> 8, crc };
        mz_bool status = tdefl_output_buffer_putter(checksum, 4, &buffer);
        if (status != MZ_TRUE) throw new std::bad_alloc();
    }

    template<typename T>
    void writeIHDR(T const& image) {
        // Write IHDR chunk.
        size_t IHDR = startChunk(IHDR_tpl, 21);
        writeUInt32BE(buffer.m_pBuf + IHDR + 8, image.width());
        writeUInt32BE(buffer.m_pBuf + IHDR + 12, image.height());

        if (sizeof(typename T::pixel_type) == 1) {
            // Paletted image
            buffer.m_pBuf[IHDR + 16] = 8; // bit depth
            buffer.m_pBuf[IHDR + 17] = 3; // color type (3 == indexed color)
        } else {
            // Full color image
            buffer.m_pBuf[IHDR + 16] = 8; // bit depth
            buffer.m_pBuf[IHDR + 17] = 6; // color type (6 == true color with alpha)
        }

        buffer.m_pBuf[IHDR + 18] = 0; // compression method
        buffer.m_pBuf[IHDR + 19] = 0; // filter method
        buffer.m_pBuf[IHDR + 20] = 0; // interlace method
        finishChunk(IHDR);
    }

    void writePLTE(std::vector<rgb> const& palette) {
        // Write PLTE chunk.
        size_t PLTE = startChunk(PLTE_tpl, 8);
        mz_uint8 *colors = const_cast<mz_uint8 *>(reinterpret_cast<const mz_uint8 *>(&palette[0]));
        mz_bool status = tdefl_output_buffer_putter(colors, palette.size() * 3, &buffer);
        if (status != MZ_TRUE) throw new std::bad_alloc();
        finishChunk(PLTE);
    }

    template<typename T>
    void writeIDAT(T const& image) {
        // Write IDAT chunk.
        size_t IDAT = startChunk(IDAT_tpl, 8);
        mz_uint8 filter_type = 0;
        tdefl_status status;
        int stride = image.width() * sizeof(typename T::pixel_type);

        for (unsigned int y = 0; y < image.height(); y++) {
            // Write filter_type
            status = tdefl_compress_buffer(compressor, &filter_type, 1, TDEFL_NO_FLUSH);
            if (status != TDEFL_STATUS_OKAY) throw new std::runtime_error("failed to compress image");

            // Write scanline
            status = tdefl_compress_buffer(compressor, (mz_uint8 *)image.getRow(y), stride, TDEFL_NO_FLUSH);
            if (status != TDEFL_STATUS_OKAY) throw new std::runtime_error("failed to compress image");
        }

        status = tdefl_compress_buffer(compressor, NULL, 0, TDEFL_FINISH);
        if (status != TDEFL_STATUS_DONE) throw new std::runtime_error("failed to compress image");

        finishChunk(IDAT);
    }

    void writeIEND() {
        // Write IEND chunk.
        size_t IEND = startChunk(IEND_tpl, 8);
        finishChunk(IEND);
    }

public:
    void compress(image_data_32 const& image) {
        initialize();
        writeIHDR(image);
        writeIDAT(image);
        writeIEND();
    }

    void compress(image_data_8 const& image, std::vector<rgb> const& palette) {
        initialize();
        writeIHDR(image);
        writePLTE(palette);
        writeIDAT(image);
        writeIEND();
    }

    void toStream(std::ostream& stream) {
        stream.write((char *)buffer.m_pBuf, buffer.m_size);
    }

private:
    mz_uint flags;
    tdefl_compressor *compressor;
    tdefl_output_buffer buffer;

    static const mz_uint8 preamble[];
    static const mz_uint8 IHDR_tpl[];
    static const mz_uint8 PLTE_tpl[];
    static const mz_uint8 IDAT_tpl[];
    static const mz_uint8 IEND_tpl[];
};

const mz_uint8 PNGWriter::preamble[] = {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a
};

const mz_uint8 PNGWriter::IHDR_tpl[] = {
    0x00, 0x00, 0x00, 0x0D, // chunk length
    'I', 'H', 'D', 'R',     // "IHDR"
    0x00, 0x00, 0x00, 0x00, // image width (4 bytes)
    0x00, 0x00, 0x00, 0x00, // image height (4 bytes)
    0x00,                   // bit depth (1 byte)
    0x00,                   // color type (1 byte)
    0x00,                   // compression method (1 byte), has to be 0
    0x00,                   // filter method (1 byte)
    0x00                    // interlace method (1 byte)
};

const mz_uint8 PNGWriter::PLTE_tpl[] = {
    0x00, 0x00, 0x00, 0x00, // chunk length
    'P', 'L', 'T', 'E'      // "IDAT"
};

const mz_uint8 PNGWriter::IDAT_tpl[] = {
    0x00, 0x00, 0x00, 0x00, // chunk length
    'I', 'D', 'A', 'T'      // "IDAT"
};

const mz_uint8 PNGWriter::IEND_tpl[] = {
    0x00, 0x00, 0x00, 0x00, // chunk length
    'I', 'E', 'N', 'D'      // "IEND"
};

}

#endif
