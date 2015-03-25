var mapnik = require('mapnik');
module.exports = mapnik.blend;
module.exports.Palette = mapnik.Palette;
module.exports.rgb2hsl2 = mapnik.rgb2hsl;
module.exports.hsl2rgb2 = mapnik.hsl2rgb;
module.exports.rgb2hsl = mapnik.rgb2hsl;
module.exports.hsl2rgb = mapnik.hsl2rgb;

var Palette = module.exports.Palette;
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

module.exports.parseTintStringOld = function(str) {
    if (!str.length) return {};

    var options = {};
    var hex = str.match(/^#?([0-9a-f]{6})$/i);
    if (hex) {
        var hsl = mapnik.rgb2hsl(
            parseInt(hex[1].substring(0, 2), 16),
            parseInt(hex[1].substring(2, 4), 16),
            parseInt(hex[1].substring(4, 6), 16)
        );
        options.hue = hsl[0]*365;
        options.saturation = hsl[1]*100;
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

module.exports.parseTintString = function(str) {
    if (!str || !str.length) return {};

    var options = {};
    var hex = str.match(/^#?([0-9a-f]{6})$/i);
    if (hex) {
        var hsl = mapnik.rgb2hsl(
            parseInt(hex[1].substring(0, 2), 16),
            parseInt(hex[1].substring(2, 4), 16),
            parseInt(hex[1].substring(4, 6), 16)
        );
        options.h = [hsl[0],hsl[0]]
        options.s = [hsl[1],hsl[1]];
        // Map midpoint grey to the color value, stretching values to
        // preserve white/black range. Will preserve good contrast and
        // midtone color at the cost of clipping extreme light/dark values.
        var l = hsl[2];
        var y0,y1;
        if (l > 0.5) {
            y0 = 0;
            y1 = l * 2;
        } else {
            y0 = l - (1-l);
            y1 = 1;
        }
        options.l = [y0,y1];
    } else {
        var parts = str.split(';');
        var split_opt = function(opt) {
            if (opt.indexOf('x') > -1) {
                var pair = opt.split("x");
                return [parseFloat(pair[0]),parseFloat(pair[1])];
            } else {
                var value = parseFloat(opt);
                return [value,value];
            }
        }
        if (parts.length > 0) options.h = split_opt(parts[0]);
        if (parts.length > 1) options.s = split_opt(parts[1]);
        if (parts.length > 2) options.l = split_opt(parts[2]);
        if (parts.length > 3) options.a = split_opt(parts[3]);
    }

    return options;
};

module.exports.upgradeTintString = function(old,round) {
    if (!old || !old.length) return old;
    if (old.match(/^#?([0-9a-f]{6})$/i) || old.indexOf('x') !== -1) return old;
    var new_tint = '';
    var parts = old.split(';');
    if (parts.length > 0) {
        var val = parseInt(parts[0],10)/365.0;
        if (round) {
            val = val.toFixed(round);
        }
        new_tint += val + 'x' + val;
    }
    if (parts.length > 1) {
        var val = parseInt(parts[1],10)/100.0;
        if (round) {
            val = val.toFixed(round);
        }
        new_tint += ';' + val + 'x' + val;
    }
    var l = ''
    if (parts.length > 2) {
        var val = parseFloat(parts[2]);
        if (round) {
            val = val.toFixed(round);
        }
        if (parts.length > 3) {
            l += ';' + val + 'x';
        } else {
            // NOTE: old style would default to 1 if y1 was not provided
            l += ';' + val + 'x' + 1;
        }
    }
    if (parts.length > 3) {
        var val = parseFloat(parts[3]);
        if (round) {
            val = val.toFixed(round);
        }
        l += val;
    }
    new_tint += l;
    if (parts.length > 4) {
        var val = parseFloat(parts[4]);
        if (round) {
            val = val.toFixed(round);
        }
        new_tint += ';0x' + val;
    }
    return new_tint;
}
