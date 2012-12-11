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

rgba_palette::rgba_palette(std::string const& pal, palette_type type)
    : colors_(0)
{
#ifdef USE_DENSE_HASH_MAP
    color_hashmap_.set_empty_key(-1);
#endif
    parse(pal, type);
}

rgba_palette::rgba_palette()
    : colors_(0)
{
#ifdef USE_DENSE_HASH_MAP
    color_hashmap_.set_empty_key(-1);
#endif
}

const std::vector<rgb>& rgba_palette::palette() const
{
    return rgb_pal_;
}

const std::vector<unsigned>& rgba_palette::alphaTable() const
{
    return alpha_pal_;
}

bool rgba_palette::valid() const
{
    return colors_ > 0;
}

// return color index in returned earlier palette
unsigned char rgba_palette::quantize(unsigned val) const
{
    unsigned char index = 0;
    if (colors_ == 1) return index;

    rgba_hash_table::iterator it = color_hashmap_.find(val);
    if (it != color_hashmap_.end())
    {
        index = it->second;
    }
    else
    {
        rgba c(val);
        int dr, dg, db, da;
        int dist, newdist, dist_add;

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
            dist_add = dr+db+dg+da;
            // stop criteria based on properties of used sorting
            if ((dist_add * dist_add / 4 > dist))
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
            dist_add = dr+db+dg+da;
            // stop criteria based on properties of used sorting
            if ((dist_add * dist_add / 4 > dist))
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
        color_hashmap_[val] = index;
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

#ifdef USE_DENSE_HASH_MAP
    color_hashmap_.resize((colors_*2));
#endif
    color_hashmap_.clear();

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

    target->Set(String::NewSymbol("Palette"), constructor->GetFunction());
}

Palette::Palette(std::string const& palette, rgba_palette::palette_type type) :
    ObjectWrap(),
    palette_(new rgba_palette(palette, type)) {}

Palette::~Palette() {
}

Handle<Value> Palette::New(const Arguments& args) {
    HandleScope scope;

    if (!args.IsConstructCall()) {
        return ThrowException(String::New("Cannot call constructor as function, you need to use 'new' keyword"));
    }

    std::string palette;
    rgba_palette::palette_type type = rgba_palette::PALETTE_RGBA;
    if (args.Length() >= 1) {
        if (args[0]->IsString()) {
            String::AsciiValue obj(args[0]->ToString());
            palette = std::string(*obj, obj.length());
        }
        else if (node::Buffer::HasInstance(args[0])) {
            Local<Object> obj = args[0]->ToObject();
            palette = std::string(node::Buffer::Data(obj), node::Buffer::Length(obj));
        }
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

