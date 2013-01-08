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

#ifndef NODE_BLEND_SRC_PALETTE_H
#define NODE_BLEND_SRC_PALETTE_H

// node
#include <v8.h>
#include <node.h>
#include <node_object_wrap.h>

// stl
#include <vector>
#include <map>
#include <iostream>
#include <tr1/memory>

#ifdef USE_DENSE_HASH_MAP
    #include <sparsehash/dense_hash_map>
    typedef google::dense_hash_map<unsigned int, unsigned char> rgba_hash_table;
#else
    #warning compiling without dense_hash_map
    #include <tr1/unordered_map>
    typedef std::tr1::unordered_map<unsigned int, unsigned char> rgba_hash_table;
#endif


#define U2RED(x) ((x)&0xff)
#define U2GREEN(x) (((x)>>8)&0xff)
#define U2BLUE(x) (((x)>>16)&0xff)
#define U2ALPHA(x) (((x)>>24)&0xff)



typedef unsigned char byte;
struct rgba;

struct rgb {
    byte r;
    byte g;
    byte b;

    inline rgb(byte r_, byte g_, byte b_) : r(r_), g(g_), b(b_) {}
    rgb(rgba const& c);

    inline rgb(unsigned const& c) {
        r = U2RED(c);
        g = U2GREEN(c);
        b = U2BLUE(c);
    }

    inline bool operator==(const rgb& y) const
    {
        return r == y.r && g == y.g && b == y.b;
    }
};

struct rgba
{
    byte r;
    byte g;
    byte b;
    byte a;

    inline rgba(byte r_, byte g_, byte b_, byte a_)
        : r(r_), g(g_), b(b_), a(a_) {}

    inline rgba(rgb const& c)
        : r(c.r), g(c.g), b(c.b), a(0xFF) {}

    inline rgba(unsigned const& c) {
        r = U2RED(c);
        g = U2GREEN(c);
        b = U2BLUE(c);
        a = U2ALPHA(c);
    }

    inline bool operator==(const rgba& y) const
    {
        return r == y.r && g == y.g && b == y.b && a == y.a;
    }

    inline operator unsigned() const
    {
        return r | (g << 8) | (b << 16) | (a << 24);
    }

    // ordering by mean(a,r,g,b), a, r, g, b
    struct mean_sort_cmp
    {
        bool operator() (const rgba& x, const rgba& y) const;
    };
};


class rgba_palette {
public:
    enum palette_type { PALETTE_RGBA = 0, PALETTE_RGB = 1, PALETTE_ACT = 2 };

    explicit rgba_palette(std::string const& pal, palette_type type = PALETTE_RGBA);
    rgba_palette();

    const std::vector<rgb>& palette() const;
    const std::vector<unsigned>& alphaTable() const;

    unsigned char quantize(unsigned c) const;

    bool valid() const;

private:
    void parse(std::string const& pal, palette_type type);
    rgba_palette(const rgba_palette&);
    const rgba_palette& operator=(const rgba_palette&);

private:
    std::vector<rgba> sorted_pal_;
    mutable rgba_hash_table color_hashmap_;

    unsigned colors_;
    std::vector<rgb> rgb_pal_;
    std::vector<unsigned> alpha_pal_;
};


typedef std::tr1::shared_ptr<rgba_palette> palette_ptr;

class Palette : public node::ObjectWrap {
public:
    static v8::Persistent<v8::FunctionTemplate> constructor;

    explicit Palette(std::string const& palette, rgba_palette::palette_type type);
    static void Initialize(v8::Handle<v8::Object> target);
    static v8::Handle<v8::Value> New(const v8::Arguments &args);

    static v8::Handle<v8::Value> ToString(const v8::Arguments& args);
    static v8::Handle<v8::Value> ToBuffer(const v8::Arguments& args);

    inline palette_ptr palette() { return palette_; }

private:
    ~Palette();
    palette_ptr palette_;
};

#endif
