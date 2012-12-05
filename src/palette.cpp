/*****************************************************************************
 *
 * This file is part of Mapnik (c++ mapping toolkit)
 *
 * Copyright (C) 2011 Artem Pavlenko
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *****************************************************************************/

#include "palette.hpp"
#include "image_data.hpp"
#include "png_io.hpp"

// node
#include <node_buffer.h>

// stl
#include <vector>
#include <iomanip>
#include <sstream>
#include <algorithm>


using namespace v8;

rgb::rgb(rgba const& c)
    : r(c.r), g(c.g), b(c.b) {}

// ordering by mean(a,r,g,b), a, r, g, b
bool rgba::mean_sort_cmp::operator() (const rgba& x, const rgba& y) const
{
    int t1 = (int)x.a + x.r + x.g + x.b;
    int t2 = (int)y.a + y.r + y.g + y.b;
    if (t1 != t2) return t1 < t2;

    // https://github.com/mapnik/mapnik/issues/1087
    if (x.a != y.a) return x.a < y.a;
    if (x.r != y.r) return x.r < y.r;
    if (x.g != y.g) return x.g < y.g;
    return x.b < y.b;
}

std::size_t rgba::hash_func::operator()(rgba const& p) const
{
    return ((std::size_t)p.r * 33023 + (std::size_t)p.g * 30013 +
            (std::size_t)p.b * 27011 + (std::size_t)p.a * 24007) % 21001;
}


rgba_palette::rgba_palette(std::string const& pal, palette_type type)
    : colors_(0)
{
    parse(pal, type);
}

rgba_palette::rgba_palette()
    : colors_(0) {}

const std::vector<rgb>& rgba_palette::palette()
{
    return rgb_pal_;
}

const std::vector<unsigned>& rgba_palette::alphaTable()
{
    return alpha_pal_;
}

bool rgba_palette::valid() const
{
    return colors_ > 0;
}

// return color index in returned earlier palette
unsigned char rgba_palette::quantize(rgba const& c) const
{
    unsigned char index = 0;
    if (colors_ == 1) return index;

    rgba_hash_table::iterator it = color_hashmap_.find(c);
    if (it != color_hashmap_.end())
    {
        index = it->second;
    }
    else
    {
        int dr, dg, db, da;
        int dist, newdist;

        // find closest match based on mean of r,g,b,a
        std::vector<rgba>::const_iterator pit =
            std::lower_bound(sorted_pal_.begin(), sorted_pal_.end(), c, rgba::mean_sort_cmp());
        index = pit - sorted_pal_.begin();
        if (index == sorted_pal_.size()) index--;

        dr = sorted_pal_[index].r - c.r;
        dg = sorted_pal_[index].g - c.g;
        db = sorted_pal_[index].b - c.b;
        da = sorted_pal_[index].a - c.a;
        dist = dr*dr + dg*dg + db*db + da*da;
        int poz = index;

        // search neighbour positions in both directions for better match
        for (int i = poz - 1; i >= 0; i--)
        {
            dr = sorted_pal_[i].r - c.r;
            dg = sorted_pal_[i].g - c.g;
            db = sorted_pal_[i].b - c.b;
            da = sorted_pal_[i].a - c.a;
            // stop criteria based on properties of used sorting
            if ((dr+db+dg+da) * (dr+db+dg+da) / 4 > dist)
            {
                break;
            }
            newdist = dr*dr + dg*dg + db*db + da*da;
            if (newdist < dist)
            {
                index = i;
                dist = newdist;
            }
        }

        for (unsigned i = poz + 1; i < sorted_pal_.size(); i++)
        {
            dr = sorted_pal_[i].r - c.r;
            dg = sorted_pal_[i].g - c.g;
            db = sorted_pal_[i].b - c.b;
            da = sorted_pal_[i].a - c.a;
            // stop criteria based on properties of used sorting
            if ((dr+db+dg+da) * (dr+db+dg+da) / 4 > dist)
            {
                break;
            }
            newdist = dr*dr + dg*dg + db*db + da*da;
            if (newdist < dist)
            {
                index = i;
                dist = newdist;
            }
        }

        // Cache found index for the color c into the hashmap.
        color_hashmap_[c] = index;
    }

    return index;
}

void rgba_palette::parse(std::string const& pal, palette_type type)
{
    unsigned length = pal.length();

    if ((type == PALETTE_RGBA && length % 4 > 0) ||
        (type == PALETTE_RGB && length % 3 > 0) ||
        (type == PALETTE_ACT && length != 772))
    {
        return;
    }

    if (type == PALETTE_ACT)
    {
        length = (pal[768] << 8 | pal[769]) * 3;
    }

    sorted_pal_.clear();
    color_hashmap_.clear();
    rgb_pal_.clear();
    alpha_pal_.clear();

    if (type == PALETTE_RGBA)
    {
        for (unsigned i = 0; i < length; i += 4)
        {
            sorted_pal_.push_back(rgba(pal[i], pal[i + 1], pal[i + 2], pal[i + 3]));
        }
    }
    else
    {
        for (unsigned i = 0; i < length; i += 3)
        {
            sorted_pal_.push_back(rgba(pal[i], pal[i + 1], pal[i + 2], 0xFF));
        }
    }

    // Make sure we have at least one entry in the palette.
    if (sorted_pal_.size() == 0)
    {
        sorted_pal_.push_back(rgba(0, 0, 0, 0));
    }

    colors_ = sorted_pal_.size();

    // Sort palette for binary searching in quantization
    std::sort(sorted_pal_.begin(), sorted_pal_.end(), rgba::mean_sort_cmp());

    // Insert all palette colors into the hashmap and into the palette vectors.
    for (unsigned i = 0; i < colors_; i++)
    {
        rgba c = sorted_pal_[i];
        color_hashmap_[c] = i;
        rgb_pal_.push_back(rgb(c));
        if (c.a < 0xFF)
        {
            alpha_pal_.push_back(c.a);
        }
    }
}

// ------- node implementation -------


Persistent<FunctionTemplate> Palette::constructor;

void Palette::Initialize(Handle<Object> target) {
    HandleScope scope;

    constructor = Persistent<FunctionTemplate>::New(FunctionTemplate::New(Palette::New));
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(String::NewSymbol("Palette"));

    NODE_SET_PROTOTYPE_METHOD(constructor, "toString", ToString);
    NODE_SET_PROTOTYPE_METHOD(constructor, "toBuffer", ToBuffer);
    NODE_SET_PROTOTYPE_METHOD(constructor, "encode", encode);

    target->Set(String::NewSymbol("Palette"), constructor->GetFunction());
}

Palette::Palette(std::string const& palette, rgba_palette::palette_type type) :
    ObjectWrap(),
    palette_(new rgba_palette(palette, type)) {}

Palette::Palette() :
    ObjectWrap() {}

Palette::~Palette() {
}

Handle<Value> Palette::New(const Arguments& args) {
    HandleScope scope;

    if (!args.IsConstructCall()) {
        return ThrowException(String::New("Cannot call constructor as function, you need to use 'new' keyword"));
    }

    if (args.Length() >= 1) {
        std::string palette;
        rgba_palette::palette_type type = rgba_palette::PALETTE_RGBA;
        if (args[0]->IsString()) {
            String::AsciiValue obj(args[0]->ToString());
            palette = std::string(*obj, obj.length());
        }
        else if (node::Buffer::HasInstance(args[0])) {
            Local<Object> obj = args[0]->ToObject();
            palette = std::string(node::Buffer::Data(obj), node::Buffer::Length(obj));
        }
        if (args.Length() >= 2) {
            if (args[1]->IsString()) {
                std::string obj = *String::Utf8Value(args[1]->ToString());
                if (obj == "rgb") type = rgba_palette::PALETTE_RGB;
                else if (obj == "act") type = rgba_palette::PALETTE_ACT;
            }
        }
        if (!palette.length()) {
            return ThrowException(Exception::TypeError(
                String::New("First parameter must be a palette string")));
        }
        Palette* p = new Palette(palette, type);
        if (!p->palette()->valid()) {
            delete p;
            return ThrowException(Exception::TypeError(String::New("Invalid palette length")));
        } else {
            p->Wrap(args.This());
            return args.This();
        }
    } else {
        Palette* p = new Palette();
        p->Wrap(args.This());
        return args.This();
    }
}

Handle<Value> Palette::ToString(const Arguments& args)
{
    HandleScope scope;
    palette_ptr p = ObjectWrap::Unwrap<Palette>(args.This())->palette_;

    const std::vector<rgb>& colors = p->palette();
    unsigned length = colors.size();
    const std::vector<unsigned>& alpha = p->alphaTable();
    unsigned alphaLength = alpha.size();

    std::ostringstream str("");
    str << "[Palette " << length;
    if (length == 1) str << " color";
    else str << " colors";

    str << std::hex << std::setfill('0');

    for (unsigned i = 0; i < length; i++) {
        str << " #";
        str << std::setw(2) << (unsigned)colors[i].r;
        str << std::setw(2) << (unsigned)colors[i].g;
        str << std::setw(2) << (unsigned)colors[i].b;
        if (i < alphaLength) str << std::setw(2) << alpha[i];
    }

    str << "]";
    return scope.Close(String::New(str.str().c_str()));
}

Handle<Value> Palette::ToBuffer(const Arguments& args)
{
    HandleScope scope;

    palette_ptr p = ObjectWrap::Unwrap<Palette>(args.This())->palette_;

    const std::vector<rgb>& colors = p->palette();
    unsigned length = colors.size();
    const std::vector<unsigned>& alpha = p->alphaTable();
    unsigned alphaLength = alpha.size();

    char palette[256 * 4];
    for (unsigned i = 0, pos = 0; i < length; i++) {
        palette[pos++] = colors[i].r;
        palette[pos++] = colors[i].g;
        palette[pos++] = colors[i].b;
        palette[pos++] = (i < alphaLength) ? alpha[i] : 0xFF;
    }

    node::Buffer *buffer = node::Buffer::New(palette, length * 4);
    return scope.Close(buffer->handle_);
}

typedef struct {
    uv_work_t request;
    node::Buffer* im;
    Palette * palette;
    bool error;
    std::string error_name;
    Persistent<Function> cb;
    std::ostringstream stream;
} encode_baton_t;

Handle<Value> Palette::encode(const Arguments& args)
{
    HandleScope scope;

    Palette* p = ObjectWrap::Unwrap<Palette>(args.This());
    node::Buffer* im = ObjectWrap::Unwrap<node::Buffer>(args[0]->ToObject());

    Local<Value> callback = args[args.Length()-1];
    if (!args[args.Length()-1]->IsFunction())
        return ThrowException(Exception::TypeError(
                                  String::New("last argument must be a callback function")));

    encode_baton_t *closure = new encode_baton_t();
    closure->request.data = closure;
    closure->im = im;
    closure->palette = p;
    closure->error = false;
    closure->cb = Persistent<Function>::New(Handle<Function>::Cast(callback));
    uv_queue_work(uv_default_loop(), &closure->request, EIO_Encode, EIO_AfterEncode);
    //im->Ref();
    p->Ref();
    return Undefined();
}

void Palette::EIO_Encode(uv_work_t* req)
{
    encode_baton_t *closure = static_cast<encode_baton_t *>(req->data);

    try {
        image_data_32 image(256, 256, (unsigned int*)node::Buffer::Data(closure->im));
        save_as_png8_pal(closure->stream, image, *closure->palette->palette(), 3, Z_DEFAULT_STRATEGY, false);
    }
    catch (std::exception & ex)
    {
        closure->error = true;
        closure->error_name = ex.what();
    }
    catch (...)
    {
        closure->error = true;
        closure->error_name = "unknown exception happened when encoding image: please file bug report";
    }
}

void Palette::EIO_AfterEncode(uv_work_t* req)
{
    HandleScope scope;

    encode_baton_t *closure = static_cast<encode_baton_t *>(req->data);

    TryCatch try_catch;

    if (closure->error) {
        Local<Value> argv[1] = { Exception::Error(String::New(closure->error_name.c_str())) };
        closure->cb->Call(Context::GetCurrent()->Global(), 1, argv);
    }
    else
    {
        std::string result = closure->stream.str();
        node::Buffer *retbuf = node::Buffer::New((char*)result.data(),result.size());
        Local<Value> argv[2] = { Local<Value>::New(Null()), Local<Value>::New(retbuf->handle_) };
        closure->cb->Call(Context::GetCurrent()->Global(), 2, argv);
    }

    if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
    }

    closure->palette->Unref();
    closure->cb.Dispose();
    delete closure;
}

