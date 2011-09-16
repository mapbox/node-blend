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
#include <queue>

typedef std::pair<unsigned char*, size_t> ImageBuffer;
typedef std::vector<ImageBuffer> ImageBuffers;
typedef v8::Persistent<v8::Object> PersistentObject;

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
int EIO_Blend(eio_req* req);
int EIO_AfterBlend(eio_req* req);


struct BlendBaton {
    v8::Persistent<v8::Function> callback;
    ImageBuffers buffers;

    bool error;
    std::string message;
    std::vector<std::string> warnings;

    BlendFormat format;
    int quality;
    bool reencode;

    unsigned char* result;
    size_t length;
    size_t max;

    BlendBaton(v8::Handle<v8::Function> cb, BlendFormat fmt, int qlt, bool reenc)
        : error(false), format(fmt), quality(qlt), reencode(reenc), result(NULL), length(0), max(0) {
        ev_ref(EV_DEFAULT_UC);
        callback = v8::Persistent<v8::Function>::New(cb);
    }
    void add(v8::Handle<v8::Object> buffer) {
        size_t length = node::Buffer::Length(buffer);
        unsigned char* image = (unsigned char*)malloc(length);
        assert(image);
        memcpy(image, node::Buffer::Data(buffer), length);
        buffers.push_back(std::make_pair<unsigned char*, size_t>(image, length));
    }
    ~BlendBaton() {
        ev_unref(EV_DEFAULT_UC);

        ImageBuffers::iterator cur = buffers.begin();
        ImageBuffers::iterator end = buffers.end();
        for (; cur < end; cur++) {
            if ((*cur).first == result) {
                result = NULL;
            }
            free((*cur).first);
            (*cur).first = NULL;
        }
        buffers.clear();

        if (result) {
            free(result);
        }

        callback.Dispose();
    }
};

#endif