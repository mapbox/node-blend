#ifndef NODE_BLEND_SRC_BLEND_H
#define NODE_BLEND_SRC_BLEND_H

#include <v8.h>
#include <node.h>
#include <node_buffer.h>
#include <png.h>
#include <jpeglib.h>

#include <cstdlib>
#include <cstring>

#include <string>
#include <vector>
#include <tr1/memory>

#include "reader.hpp"

typedef v8::Persistent<v8::Object> PersistentObject;

struct Image {
    Image() :
        data(NULL),
        dataLength(0),
        x(0),
        y(0),
        width(0),
        height(0) {}
    PersistentObject buffer;
    unsigned char *data;
    size_t dataLength;
    int x, y;
    int width, height;
    std::auto_ptr<ImageReader> reader;
};

typedef std::tr1::shared_ptr<Image> ImagePtr;
typedef std::vector<ImagePtr> Images;

enum BlendFormat {
    BLEND_FORMAT_PNG,
    BLEND_FORMAT_JPEG
};

#define TRY_CATCH_CALL(context, callback, argc, argv)                          \
{   v8::TryCatch try_catch;                                                    \
    (callback)->Call((context), (argc), (argv));                               \
    if (try_catch.HasCaught()) {                                               \
        node::FatalException(try_catch);                                       \
    }                                                                          }

#define TYPE_EXCEPTION(message)                                                \
    ThrowException(Exception::TypeError(String::New(message)))

v8::Handle<v8::Value> Blend(const v8::Arguments& args);
void Work_Blend(uv_work_t* req);
void Work_AfterBlend(uv_work_t* req);


struct BlendBaton {
    uv_work_t request;
    v8::Persistent<v8::Function> callback;
    Images images;

    std::string message;
    std::vector<std::string> warnings;

    int quality;
    BlendFormat format;
    bool reencode;
    int width;
    int height;
    unsigned int matte;

    unsigned char* result;
    size_t resultLength;
    size_t max;

    BlendBaton() :
        quality(0),
        format(BLEND_FORMAT_PNG),
        reencode(false),
        width(0),
        height(0),
        matte(0),
        result(NULL),
        resultLength(0),
        max(0)
    {
        this->request.data = this;
        uv_ref(uv_default_loop());
    }

    ~BlendBaton() {
        uv_unref(uv_default_loop());

        for (Images::iterator cur = images.begin(); cur != images.end(); cur++) {
            (*cur)->buffer.Dispose();
        }

        // Note: THe result buffer is freed by the node Buffer's free callback

        callback.Dispose();
    }
};

#endif