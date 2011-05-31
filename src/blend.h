#ifndef NODE_BLEND_SRC_BLEND_H
#define NODE_BLEND_SRC_BLEND_H

#include <v8.h>
#include <node.h>
#include <png.h>

#include <cstdlib>
#include <cstring>

#include <string>
#include <queue>

typedef std::pair<unsigned char*, size_t> ImageBuffer;
typedef std::vector<ImageBuffer> ImageBuffers;
typedef v8::Persistent<v8::Object> PersistentObject;

#define TRY_CATCH_CALL(context, callback, argc, argv)                          \
{   v8::TryCatch try_catch;                                                    \
    (callback)->Call((context), (argc), (argv));                               \
    if (try_catch.HasCaught()) {                                               \
        node::FatalException(try_catch);                                       \
    }                                                                          }

v8::Handle<v8::Value> Blend(const v8::Arguments& args);
int EIO_Blend(eio_req *req);
int EIO_AfterBlend(eio_req *req);

#endif