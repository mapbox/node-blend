#include <ostream>

extern "C"
{
#include <stdio.h>
#include <jpeglib.h>

}

#include "jpeg_io.hpp"
#include "reader.hpp"


void optimize_jpeg(std::ostream& file, JPEGImageReader *layer)
{
    jpeg_decompress_struct& dinfo = layer->decompressStruct();
    jpeg_compress_struct cinfo;
    jpeg_error_mgr jdsterr;

    cinfo.err = jpeg_std_error(&jdsterr);
    jpeg_create_compress(&cinfo);

    cinfo.dest = (jpeg_destination_mgr *)(*cinfo.mem->alloc_small)
        ((j_common_ptr) &cinfo, JPOOL_PERMANENT, sizeof(dest_mgr));
    dest_mgr *dest = (dest_mgr*) cinfo.dest;
    dest->pub.init_destination = init_destination;
    dest->pub.empty_output_buffer = empty_output_buffer;
    dest->pub.term_destination = term_destination;
    dest->out = &file;

    // Set optimization parameters
    cinfo.optimize_coding = TRUE;
    jpeg_simple_progression(&cinfo);

    // Read source file as DCT coefficients
    jvirt_barray_ptr *coef_arrays = jpeg_read_coefficients(&dinfo);

    // Initialize destination compression parameters from source values
    jpeg_copy_critical_parameters(&dinfo, &cinfo);

    // Start compressor (note no image data is actually written here)
    jpeg_write_coefficients(&cinfo, coef_arrays);

    // Finish compression and release memory
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    (void)jpeg_finish_decompress(&dinfo);
    jpeg_destroy_decompress(&dinfo);

    // exit(jsrcerr.num_warnings + jdsterr.num_warnings ? EXIT_WARNING : EXIT_SUCCESS);
}
