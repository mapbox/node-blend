#include <string.h>
#include <v8.h>
#include <node.h>
#include <node_buffer.h>

#include "blend.h"
#include "reader.h"

using namespace v8;
using namespace node;

struct BlendBaton {
    Persistent<Function> callback;
    ImageBuffers buffers;

    bool error;

    unsigned char* result;
    size_t length;
    size_t max;

    BlendBaton(Handle<Function> cb)
        : error(false), result(NULL), length(0), max(0) {
        ev_ref(EV_DEFAULT_UC);
        callback = Persistent<Function>::New(cb);
    }
    void add(Handle<Object> buffer) {
        size_t length = Buffer::Length(buffer);
        unsigned char* image = (unsigned char*)malloc(length);
        assert(image);
        memcpy(image, Buffer::Data(buffer), length);
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

Handle<Value> ThrowOrCall(Handle<Function> callback, const char* message) {
    if (callback.IsEmpty()) {
        return ThrowException(Exception::TypeError(String::New(message)));
    } else {
        Local<Value> argv[] = { Exception::TypeError(String::New(message)) };
        TRY_CATCH_CALL(Context::GetCurrent()->Global(), callback, 1, argv)
        return Undefined();
    }
}

Handle<Value> Blend(const Arguments& args) {
    HandleScope scope;

    if (args.Length() < 2 || !args[1]->IsFunction()) {
        return ThrowException(Exception::TypeError(String::New("Second argument must be a function")));
    }
    Local<Function> callback = Local<Function>::Cast(args[1]);

    if (args.Length() < 1 || !args[0]->IsArray()) {
        return ThrowOrCall(callback, "First argument must be an array of Buffers.");
    }
    Local<Array> buffers = Local<Array>::Cast(args[0]);

    uint32_t length = buffers->Length();

    if (length < 1) {
        return ThrowOrCall(callback, "First argument must contain at least one Buffer.");
    } else if (length == 1) {
        if (!Buffer::HasInstance(buffers->Get(0))) {
            return ThrowOrCall(callback, "All elements must be Buffers.");
        } else {
            // Directly pass through buffer if it's the only one.
            Local<Value> argv[] = {
                Local<Value>::New(Null()),
                Local<Value>::New(buffers->Get(0))
            };
            TRY_CATCH_CALL(Context::GetCurrent()->Global(), callback, 2, argv);
        }
    } else {
        BlendBaton* baton = new BlendBaton(callback);
        for (uint32_t i = 0; i < length; i++) {
            if (!Buffer::HasInstance(buffers->Get(i))) {
                delete baton;
                return ThrowOrCall(callback, "All elements must be Buffers.");
            } else {
                baton->add(buffers->Get(i)->ToObject());
            }
        }

        eio_custom(EIO_Blend, EIO_PRI_DEFAULT, EIO_AfterBlend, baton);
    }

    return scope.Close(Undefined());
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


void Blend_Encode(unsigned const char* source, BlendBaton* baton,
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

inline void Blend_CompositeTopDown(unsigned int* images[], int size, unsigned long width, unsigned long height) {
    size_t length = width * height;
    for (long px = length - 1; px >= 0; px--) {
        // Starting pixel
        unsigned int abgr = images[0][px];

        // Skip if topmost pixel is opaque.
        if (abgr >= 0xFF000000) continue;

        for (int i = 1; i < size; i++) {
            if (images[i][px] <= 0x00FFFFFF) {
                // Lower pixel is fully transparent.
                continue;
            } else if (abgr <= 0x00FFFFFF) {
                // Upper pixel is fully transparent.
                abgr = images[i][px];
            } else {
                // Both pixels have transparency.
                unsigned int rgba0 = images[i][px];
                unsigned int rgba1 = abgr;

                // From http://trac.mapnik.org/browser/trunk/include/mapnik/graphics.hpp#L337
                unsigned a1 = (rgba1 >> 24) & 0xff;
                unsigned r1 = rgba1 & 0xff;
                unsigned g1 = (rgba1 >> 8 ) & 0xff;
                unsigned b1 = (rgba1 >> 16) & 0xff;

                unsigned a0 = (rgba0 >> 24) & 0xff;
                unsigned r0 = (rgba0 & 0xff) * a0;
                unsigned g0 = ((rgba0 >> 8 ) & 0xff) * a0;
                unsigned b0 = ((rgba0 >> 16) & 0xff) * a0;

                a0 = ((a1 + a0) << 8) - a0*a1;

                r0 = ((((r1 << 8) - r0) * a1 + (r0 << 8)) / a0);
                g0 = ((((g1 << 8) - g0) * a1 + (g0 << 8)) / a0);
                b0 = ((((b1 << 8) - b0) * a1 + (b0 << 8)) / a0);
                a0 = a0 >> 8;
                abgr = (a0 << 24)| (b0 << 16) | (g0 << 8) | (r0);
            }
            if (abgr >= 0xFF000000) break;
        }

        // Merge pixel back.
        images[0][px] = abgr;
    }
}

int EIO_Blend(eio_req *req) {
    BlendBaton* baton = static_cast<BlendBaton*>(req->data);

    int total = baton->buffers.size();
    int size = 0;
    unsigned int* images[total];
    for (int i = 0; i < total; i++) images[i] = NULL;

    unsigned long width = 0;
    unsigned long height = 0;
    bool alpha = true;

    // Iterate from the last to first image.
    ImageBuffers::reverse_iterator image = baton->buffers.rbegin();
    ImageBuffers::reverse_iterator end = baton->buffers.rend();
    for (; image < end; image++) {
        ImageReader* layer = ImageReader::create((*image).first, (*image).second);

        // Skip invalid images.
        if (layer == NULL) continue;

        if (size == 0) {
            width = layer->width;
            height = layer->height;
            if (!layer->alpha) {
                baton->result = (*image).first;
                baton->length = (*image).second;
                delete layer;
                break;
            }
        } else if (layer->width != width || layer->height != height) {
            baton->error = true;
            delete layer;
            break;
        }

        images[size] = (unsigned int*)layer->decode();
        size++;

        if (!layer->alpha) {
            // Skip decoding more layers.
            alpha = false;
            delete layer;
            break;
        }

        delete layer;
    }

    if (!baton->error && size) {
        Blend_CompositeTopDown(images, size, width, height);
        Blend_Encode((unsigned char*)images[0], baton, width, height, alpha);
    }

    for (int i = 0; i < size; i++) {
        if (images[i] != NULL) {
            free(images[i]);
            images[i] = NULL;
        }
    }

    return 0;
}

int EIO_AfterBlend(eio_req *req) {
    HandleScope scope;
    BlendBaton* baton = static_cast<BlendBaton*>(req->data);

    if (!baton->error && baton->result) {
        Local<Value> argv[] = {
            Local<Value>::New(Null()),
            Local<Value>::New(Buffer::New((char*)baton->result, baton->length)->handle_)
        };
        TRY_CATCH_CALL(Context::GetCurrent()->Global(), baton->callback, 2, argv);
    } else {
        Local<Value> argv[] = {
            Local<Value>::New(Exception::TypeError(String::New("Unspecified error")))
        };
        TRY_CATCH_CALL(Context::GetCurrent()->Global(), baton->callback, 1, argv);
    }

    delete baton;
    return 0;
}

extern "C" void init (Handle<Object> target) {
    NODE_SET_METHOD(target, "blend", Blend);

    target->Set(
        String::NewSymbol("libpng"),
        String::NewSymbol(PNG_LIBPNG_VER_STRING),
        static_cast<PropertyAttribute>(ReadOnly | DontDelete)
    );
}
