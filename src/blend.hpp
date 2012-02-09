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

typedef v8::Persistent<v8::Object> PersistentObject;

struct Image {
    PersistentObject buffer;
    unsigned char *data;
    size_t length;
};

typedef std::vector<Image> Images;

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

    bool error;
    std::string message;
    std::vector<std::string> warnings;

    BlendFormat format;
    int quality;
    bool reencode;

    unsigned char* result;
    size_t length;
    size_t max;

    BlendBaton() :
        error(false),
        format(BLEND_FORMAT_PNG),
        quality(0),
        reencode(false),
        result(NULL),
        length(0),
        max(0)
    {
        this->request.data = this;
        uv_ref(uv_default_loop());
    }

    ~BlendBaton() {
        uv_unref(uv_default_loop());

        for (Images::iterator cur = images.begin(); cur != images.end(); cur++) {
            cur->buffer.Dispose();
        }

        // Note: THe result buffer is freed by the node Buffer's free callback

        callback.Dispose();
    }
};

#endif