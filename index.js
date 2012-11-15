var blend = require('./lib/blend');
module.exports = blend.blend;
module.exports.Palette = blend.Palette;
module.exports.libpng = blend.libpng;
module.exports.libjpeg = blend.libjpeg;
module.exports.rgb2hsl2 = blend.rgb2hsl2;
module.exports.hsl2rgb2 = blend.hsl2rgb2;

var Palette = blend.Palette;
Palette.prototype.clone = function() {
    return new this.constructor(this.toBuffer());
};

// Accepts an array of hex strings (with 6 or 8 hex characters).
Palette.fromJSON = function(json) {
    var palette = json.map(function(str) {
        return str.length === 6 ? str + 'ff' : str;
    }).join('');
    return new Palette(new Buffer(palette, 'hex'), 'rgba');
};

module.exports.hsl2rgb = function(h, s, l) {
    if (!s) return [l * 255, l * 255, l * 255];

    var hueToRGB = function (m1, m2, h) {
        h = (h + 1) % 1;
        if (h * 6 < 1) return m1 + (m2 - m1) * h * 6;
        if (h * 2 < 1) return m2;
        if (h * 3 < 2) return m1 + (m2 - m1) * (0.66666 - h) * 6;
        return m1;
    };

    var m2 = (l <= 0.5) ? l * (s + 1) : l + s - l * s;
    var m1 = l * 2 - m2;
    return [
        parseInt(hueToRGB(m1, m2, h + 0.33333) * 255,10),
        parseInt(hueToRGB(m1, m2, h) * 255,10),
        parseInt(hueToRGB(m1, m2, h - 0.33333) * 255,10)
    ];
};

var rgb2hsl = function(r, g, b){
    r /= 255, g /= 255, b /= 255;
    var max = Math.max(r, g, b);
    var min = Math.min(r, g, b);
    var delta = max - min;
    var gamma = max + min;
    var h = 0, s = 0, l = gamma / 2;

    if (delta) {
        s = l > 0.5 ? delta / (2 - gamma) : delta / gamma;
        if (max == r && max != g) h = (g - b) / delta + (g < b ? 6 : 0);
        if (max == g && max != b) h = (b - r) / delta + 2;
        if (max == b && max != r) h = (r - g) / delta + 4;
        h /= 6;
    }

    return [h, s, l];
};
module.exports.rgb2hsl = rgb2hsl;

module.exports.parseTintString = function(str) {
    if (!str.length) return {};

    var options = {};
    var hex = str.match(/^#?([0-9a-f]{6})$/i);
    if (hex) {
        var hsl = rgb2hsl(
            parseInt(hex[1].substring(0, 2), 16),
            parseInt(hex[1].substring(2, 4), 16),
            parseInt(hex[1].substring(4, 6), 16)
        );
        options.hue = hsl[0];
        options.saturation = hsl[1];
        // Map midpoint grey to the color value, stretching values to
        // preserve white/black range. Will preserve good contrast and
        // midtone color at the cost of clipping extreme light/dark values.
        var l = hsl[2];
        if (l > 0.5) {
            options.y0 = 0;
            options.y1 = l * 2;
        } else {
            options.y0 = l - (1-l);
            options.y1 = 1;
        }
    } else {
        var parts = str.split(';');
        if (parts.length > 0) options.hue = parseFloat(parts[0]);
        if (parts.length > 1) options.saturation = parseFloat(parts[1]);
        if (parts.length > 2) options.y0 = parseFloat(parts[2]);
        if (parts.length > 3) options.y1 = parseFloat(parts[3]);
        if (parts.length > 4) options.opacity = parseFloat(parts[4]);
    }

    return options;
};
