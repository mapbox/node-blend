var blend = require('./lib/blend');
module.exports = blend.blend;
module.exports.Palette = blend.Palette;
module.exports.libpng = blend.libpng;
module.exports.libjpeg = blend.libjpeg;

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
