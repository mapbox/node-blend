var mapnik = require('mapnik');
var op = mapnik.compositeOp.src_over;

module.exports = function(layers, options, callback) {
    if (!callback) {
       callback = options;
       options = {}
    }
    if (!layers || !(layers instanceof Array)) {
        throw new Error('First argument must be an array of Buffers');
    }
    if (layers.length) {
        if (layers[0].buffer) {
            if (!(layers[0].buffer instanceof Buffer)) {
                throw new Error("All elements must be Buffers or objects with a 'buffer' property");
            }
        } else {
            if (!(layers[0] instanceof Buffer)) {
                throw new Error("All elements must be Buffers or objects with a 'buffer' property");
            }
        }
        if (options && options.format) {
            if (options.format != 'png' && options.format != 'jpeg' && options.format != 'webp') {
                throw new Error('Invalid output format');
            }
        }
    }
    // make shallow copy
    layers = layers.slice(0);
    // node-blend internally creates first canvas based on
    // first image's size if width/height are not supplied
    // this is a suboptimal way of emulating that for now
    if (!options.width || !options.height) {
        if (layers[0] instanceof Buffer && !layers[0].buffer) {
            layers[0].buffer = layers[0];
        }
        var im = new mapnik.Image.fromBytesSync(layers[0].buffer);
        options.width = im.width();
        options.height = im.height();
    }
    var canvas = new mapnik.Image(options.width, options.height);

    // set matte background color on the canvas image.
    // the color is premultiplied manually here in preparation
    // for final demultiplication later.
    if (options.matte) try {
        var color = new mapnik.Color('#' + options.matte);
        canvas.background = new mapnik.Color(
            (color.r * color.a / 255) | 0,
            (color.g * color.a / 255) | 0,
            (color.b * color.a / 255) | 0,
            color.a
        );
    } catch(err) {
        return callback(err);
    }

    compose(canvas, layers, function(err, canvas) {
        if (err) return callback(err);
        canvas.demultiply(function(err) {
            if (err) return callback(err);
            var format = '';
            if (!options.format) {
                options.format = 'png';
            }
            switch (options.format) {
            case 'jpg':
                format = 'jpeg' + (options.quality || '80');
                break;
            case 'jpeg':
                format = 'jpeg' + (options.quality || '80');
                break;
            case 'png':
                format = 'png';
                if (options.quality) {
                    format += '8:c='+options.quality;
                }
                if (options.compression) {
                    format += ':z='+options.compression;
                }
                if (options.hextree && options.hextree == true) {
                    format += ':m=h';
                }
                break;
            case 'webp':
                format = 'webp';
                if (options.quality) {
                    format += ':quality='+options.quality;
                }
                if (options.compression) {
                    format += ':method='+options.compression;
                }
                break;
            }
            canvas.encode(format, {}, function(err, buffer) {
                return callback(err,buffer,[]); // [] is for warnings
            });
        });
    });
};

function tintToString(tint) {
    var s = 'hsla(';
    if (tint.h) s += tint.h[0] + 'x' + tint.h[1] + ';'; 
    else s += '0x1;';

    if (tint.s) s += tint.s[0] + 'x' + tint.s[1] + ';';
    else s += '0x1;';

    if (tint.l) s += tint.l[0] + 'x' + tint.l[1] + ';';
    else s += '0x1;';

    if (tint.a) s += tint.a[0] + 'x' + tint.a[1] + ')';
    else s += '0x1)';
    return s;
}

function compose(canvas, layers, callback) {
    if (!layers.length) return callback(null, canvas);

    var layer = layers.shift();
    if (layer instanceof Buffer && !layer.buffer) {
        layer.buffer = layer;
    }
    var opts = {
        dx: layer.x || 0,
        dy: layer.y || 0,
        image_filters: layer.tint ? tintToString(layer.tint): '',
        comp_op: op
    };

    // Create group of layers to be composited into a stack.
    // Used to composite tiles of a static API image as individual tiles
    // before the entire canvas is composited. This compensates for the lack
    // of offset handling in mapnik.VectorTile.render().
    if (layer.group) {
        var group = [layer];
        while (layers.length && layer.group === layers[0].group) {
            group.push(layers.shift());
        }
        // Clear out values which are now applied at the group level.
        for (var i = 0; i < group.length; i++) {
            delete group[i].group;
            delete group[i].x;
            delete group[i].y;
        }

        compose(new mapnik.Image(256, 256), group, function(err, grouped) {
            if (err) return callback(err);
            canvas.composite(grouped, opts, function(err, canvas) {
                if (err) return callback(err);
                compose(canvas, layers, callback);
            });
        });
    } else if (layer.buffer instanceof mapnik.VectorTile) {
        var vtile = layer.buffer;
        vtile.render(vtile.map, canvas, vtile.opts, function(err, composed) {
            if (err) return callback(err);
            composed.premultiply(function(err) {
                if (err) return callback(err);
                compose(canvas, layers, callback);
            });
        });
    } else {
        mapnik.Image.fromBytes(layer.buffer, function(err, image) {
            if (err) return callback(err);
            image.premultiply(function(err, image) {
                if (err) return callback(err);
                canvas.composite(image, opts, function(err, canvas) {
                    if (err) return callback(err);
                    compose(canvas, layers, callback);
                });
            });
        });
    }
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

    h = h > 1 ? 1 : h < 0 ? 0 : h;
    s = s > 1 ? 1 : s < 0 ? 0 : s;
    l = l > 1 ? 1 : l < 0 ? 0 : l;
    return [h, s, l];
};
module.exports.rgb2hsl = rgb2hsl;

module.exports.parseTintStringOld = function(str) {
    if (!str.length) return {};

    var options = {};
    var hex = str.match(/^#?([0-9a-f]{6})$/i);
    if (hex) {
        var hsl = rgb2hsl(
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
        var hsl = rgb2hsl(
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
            // https://github.com/developmentseed/node-tint/blob/849e3b1fc4f73135094f913d772d12fdb2f79865/lib/tint.js#L88
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
