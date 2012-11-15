var assert = require('assert');
var util = require('util');
var blend = require('..');

var colors = [
    [255,255,255],
    [0,0,0]
]

for (var i=0;i < 1000;i++) {
    var rgb = [parseInt(Math.random()*255,10),parseInt(Math.random()*255,10),parseInt(Math.random()*255,10)];
    colors.push(rgb);
}


describe('roundtripping rgb->hsl->rgb', function() {
    colors.forEach(function(c) {
        it('parse ' + util.inspect(c), function() {
            var hsl = blend.rgb2hsl(c[0],c[1],c[2]);
            var rgb = blend.hsl2rgb(hsl[0],hsl[1],hsl[2]);
            assert.deepEqual(rgb, c);
        });
    });
});
