#ifndef NODE_BLEND_SRC_TINT_H
#define NODE_BLEND_SRC_TINT_H

#include <cmath>
#include <string>
#include <tr1/unordered_map>

#ifdef USE_DENSE_HASH_MAP
    #include <sparsehash/dense_hash_map>
    typedef google::dense_hash_map<unsigned int, unsigned> color_cache;
    typedef std::tr1::unordered_map<std::string,color_cache> hsl_cache;
#else
    #warning compiling without dense_hash_map
    typedef std::tr1::unordered_map<unsigned int, unsigned> color_cache;
    typedef std::tr1::unordered_map<std::string,color_cache> hsl_cache;
#endif

static void rgb2hsl(unsigned red, unsigned green, unsigned blue,
             double & h, double & s, double & l) {
    double r = red/255.0;
    double g = green/255.0;
    double b = blue/255.0;
    double max = std::max(r,std::max(g,b));
    double min = std::min(r,std::min(g,b));
    double delta = max - min;
    double gamma = max + min;
    h = 0.0, s = 0.0, l = gamma / 2.0;
    if (delta > 0.0) {
        s = l > 0.5 ? delta / (2.0 - gamma) : delta / gamma;
        if (max == r && max != g) h = (g - b) / delta + (g < b ? 6.0 : 0.0);
        if (max == g && max != b) h = (b - r) / delta + 2.0;
        if (max == b && max != r) h = (r - g) / delta + 4.0;
        h /= 6.0;
    }
}

static inline void faster_fmod(const double n, const double d, double & q)
{
  if (d == 0.0) {
    q = 0;
  } else {
      q = n/d;
      if (q < 0.0)
        q -= 1.0;
      q = n - static_cast<unsigned>(q)*d;
  }
}


static inline double hueToRGB(double m1, double m2, double h) {
    faster_fmod(h+1,1,h);
    if (h * 6 < 1) return m1 + (m2 - m1) * h * 6;
    if (h * 2 < 1) return m2;
    if (h * 3 < 2) return m1 + (m2 - m1) * (0.66666 - h) * 6;
    return m1;
}

static inline void hsl2rgb(double h, double s, double l,
             unsigned & r, unsigned & g, unsigned & b) {
    if (!s) {
        r = g = b = static_cast<unsigned>(l * 255);
    }
    double m2 = (l <= 0.5) ? l * (s + 1) : l + s - l * s;
    double m1 = l * 2 - m2;
    r = static_cast<unsigned>(hueToRGB(m1, m2, h + 0.33333) * 255);
    g = static_cast<unsigned>(hueToRGB(m1, m2, h) * 255);
    b = static_cast<unsigned>(hueToRGB(m1, m2, h - 0.33333) * 255);
}


struct Tinter {
    double h0;
    double h1;
    double s0;
    double s1;
    double l0;
    double l1;
    double a0;
    double a1;
    color_cache cache;
    bool debug;

    Tinter() :
      h0(0),
      h1(1),
      s0(0),
      s1(1),
      l0(0),
      l1(1),
      a0(0),
      a1(1),
      cache(255),
      debug(false) {
#ifdef USE_DENSE_HASH_MAP
        cache.set_empty_key(0);
#endif
      }

    bool is_identity() {
        return (h0 == 0 &&
                h1 == 1 &&
                s0 == 0 &&
                s1 == 1 &&
                l0 == 0 &&
                l1 == 1);
    }

    bool is_alpha_identity() {
        return (a0 == 0 &&
                a1 == 1);
    }

    /*std::string to_string() {
        std::ostringstream s;
        s << h0 << "x" << h1 << ";"
          << s0 << "x" << s1 << ";"
          << l0 << "x" << l1 << ";"
          << a0 << "x" << a1;
        return s.str();
    }*/
};


#endif
