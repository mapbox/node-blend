#include "blend.hpp"
#include "reader.hpp"
#include "writer.hpp"

#include <sstream>

using namespace v8;
using namespace node;

Handle<Value> Blend(const Arguments& args) {
    HandleScope scope;

    Local<Function> callback;
    Local<Object> options;

    if (args.Length() == 0 || !args[0]->IsArray()) {
        return TYPE_EXCEPTION("First argument must be an array of Buffers.");
    } else if (args.Length() == 1) {
        return TYPE_EXCEPTION("Second argument must be a function");
    } else if (args.Length() == 2) {
        // No options provided.
        if (!args[1]->IsFunction()) {
            return TYPE_EXCEPTION("Second argument must be a function.");
        }
        callback = Local<Function>::Cast(args[1]);
    } else if (args.Length() >= 3) {
        if (!args[1]->IsObject()) {
            return TYPE_EXCEPTION("Second argument must be a an options object.");
        }
        options = Local<Object>::Cast(args[1]);

        if (!args[2]->IsFunction()) {
            return TYPE_EXCEPTION("Third argument must be a function.");
        }
        callback = Local<Function>::Cast(args[2]);
    }


    BlendFormat format = BLEND_FORMAT_PNG;
    int quality = 0;
    bool reencode = false;

    // Validate options
    if (!options.IsEmpty()) {
        Local<Value> quality_val = options->Get(String::NewSymbol("quality"));
        if (!quality_val.IsEmpty() && quality_val->IsInt32()) {
            quality = quality_val->Int32Value();
        }

        Local<Value> format_val = options->Get(String::NewSymbol("format"));
        if (!format_val.IsEmpty() && format_val->BooleanValue()) {
            if (strcmp(*String::AsciiValue(format_val), "jpeg") == 0 ||
                    strcmp(*String::AsciiValue(format_val), "jpg") == 0) {
                format = BLEND_FORMAT_JPEG;
                if (quality == 0) quality = 80;
                else if (quality < 0 || quality > 100) {
                    return TYPE_EXCEPTION("JPEG quality is range 0-100.");
                }
            } else if (strcmp(*String::AsciiValue(format_val), "png") == 0) {
                if (quality == 1 || quality > 256) {
                    return TYPE_EXCEPTION("PNG images must be quantized between 2 and 256 colors.");
                }
            } else {
                return TYPE_EXCEPTION("Invalid output format.");
            }
        }

        reencode = options->Get(String::NewSymbol("reencode"))->BooleanValue();
    }

    Local<Array> buffers = Local<Array>::Cast(args[0]);
    uint32_t length = buffers->Length();
    if (length < 1) {
        return TYPE_EXCEPTION("First argument must contain at least one Buffer.");
    } else if (length == 1 && !reencode) {
        if (!Buffer::HasInstance(buffers->Get(0))) {
            return TYPE_EXCEPTION("All elements must be Buffers.");
        } else {
            // Directly pass through buffer if it's the only one.
            Local<Value> argv[] = {
                Local<Value>::New(Null()),
                Local<Value>::New(buffers->Get(0))
            };
            TRY_CATCH_CALL(Context::GetCurrent()->Global(), callback, 2, argv);
        }
    } else {
        BlendBaton* baton = new BlendBaton(callback, format, quality, reencode);
        for (uint32_t i = 0; i < length; i++) {
            if (!Buffer::HasInstance(buffers->Get(i))) {
                delete baton;
                return TYPE_EXCEPTION("All elements must be Buffers.");
            } else {
                baton->add(buffers->Get(i)->ToObject());
            }
        }
        baton->request.data = baton;
        uv_queue_work(uv_default_loop(), &baton->request, Work_Blend, Work_AfterBlend);
    }

    return scope.Close(Undefined());
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
                unsigned g1 = (rgba1 >> 8) & 0xff;
                unsigned b1 = (rgba1 >> 16) & 0xff;

                unsigned a0 = (rgba0 >> 24) & 0xff;
                unsigned r0 = (rgba0 & 0xff) * a0;
                unsigned g0 = ((rgba0 >> 8) & 0xff) * a0;
                unsigned b0 = ((rgba0 >> 16) & 0xff) * a0;

                a0 = ((a1 + a0) << 8) - a0 * a1;

                r0 = ((((r1 << 8) - r0) * a1 + (r0 << 8)) / a0);
                g0 = ((((g1 << 8) - g0) * a1 + (g0 << 8)) / a0);
                b0 = ((((b1 << 8) - b0) * a1 + (b0 << 8)) / a0);
                a0 = a0 >> 8;
                abgr = (a0 << 24) | (b0 << 16) | (g0 << 8) | (r0);
            }
            if (abgr >= 0xFF000000) break;
        }

        // Merge pixel back.
        images[0][px] = abgr;
    }
}

void Work_Blend(uv_work_t* req) {
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
    for (int index = total - 1; image != end; image++, index--) {
        ImageReader* layer = ImageReader::create(image->buffer, image->length);

        // Skip invalid images.
        if (layer == NULL || layer->width == 0 || layer->height == 0) {
            baton->error = true;
            baton->message = layer->message;
            delete layer;
            break;
        }

        if (size == 0) {
            width = layer->width;
            height = layer->height;

            // Short-circuit when we're not reencoding.
            if (!layer->alpha && !baton->reencode) {
                baton->result = image->buffer;
                baton->length = image->length;
                delete layer;
                break;
            }
        } else if (layer->width != width || layer->height != height) {
            baton->error = true;
            baton->message = "Image dimensions don't match";
            delete layer;
            break;
        }

        images[size] = (unsigned int*)layer->decode();
        if (images[size] == NULL) {
            // Decoding failed.
            baton->error = true;
            baton->message = layer->message;
            delete layer;
            break;
        }
        else if (layer->warnings.size()) {
            std::vector<std::string>::iterator pos = layer->warnings.begin();
            std::vector<std::string>::iterator end = layer->warnings.end();
            for (; pos != end; pos++) {
                std::ostringstream msg;
                msg << "Layer " << index << ": " << *pos;
                baton->warnings.push_back(msg.str());
            }
        }

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
        if (size > 1) {
            Blend_CompositeTopDown(images, size, width, height);
        }

        Blend_Encode((unsigned char*)images[0], baton, width, height, alpha);
    }

    for (int i = 0; i < size; i++) {
        if (images[i] != NULL) {
            free(images[i]);
            images[i] = NULL;
        }
    }
}

void Work_AfterBlend(uv_work_t* req) {
    HandleScope scope;
    BlendBaton* baton = static_cast<BlendBaton*>(req->data);

    if (!baton->error && baton->result) {
        Local<Array> warnings = Array::New();
        std::vector<std::string>::iterator pos = baton->warnings.begin();
        std::vector<std::string>::iterator end = baton->warnings.end();
        for (int i = 0; pos != end; pos++, i++) {
            warnings->Set(i, String::New((*pos).c_str()));
        }

        Local<Value> argv[] = {
            Local<Value>::New(Null()),
            Local<Value>::New(Buffer::New((char*)baton->result, baton->length)->handle_),
            Local<Value>::New(warnings)
        };
        TRY_CATCH_CALL(Context::GetCurrent()->Global(), baton->callback, 3, argv);
    } else {
        Local<Value> argv[] = {
            Local<Value>::New(Exception::Error(String::New(baton->message.c_str())))
        };
        TRY_CATCH_CALL(Context::GetCurrent()->Global(), baton->callback, 1, argv);
    }

    delete baton;
}

extern "C" void init(Handle<Object> target) {
    NODE_SET_METHOD(target, "blend", Blend);

    target->Set(
        String::NewSymbol("libpng"),
        String::NewSymbol(PNG_LIBPNG_VER_STRING),
        static_cast<PropertyAttribute>(ReadOnly | DontDelete)
    );

    target->Set(
        String::NewSymbol("libjpeg"),
        Integer::New(JPEG_LIB_VERSION),
        static_cast<PropertyAttribute>(ReadOnly | DontDelete)
    );
}
