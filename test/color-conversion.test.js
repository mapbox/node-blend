var assert = require('assert');
var util = require('util');
var blend = require('..');

var colors = [
    [255,255,255],
    [0,0,0]
]

var iterations = 30;

for (var i=0;i < iterations;i++) {
    var rgb = [parseInt(Math.random()*255,10),parseInt(Math.random()*255,10),parseInt(Math.random()*255,10)];
    colors.push(rgb);
}

function nearlyEqual(array1,array2, tolerance) {
    var r = Math.abs(array1[0] - array2[0])
    var g = Math.abs(array1[1] - array2[1])
    var b = Math.abs(array1[2] - array2[2])
    assert.ok(r <= tolerance, 'r channel differs "' + r + '" more than accepted tolerance: ' + tolerance)
    assert.ok(g <= tolerance, 'g channel differs "' + g + '" more than accepted tolerance: ' + tolerance)
    assert.ok(b <= tolerance, 'b channel differs "' + b + '" more than accepted tolerance: ' + tolerance)
}

describe('roundtripping rgb->hsl->rgb in both js and c++', function() {
    colors.forEach(function(c) {
        it('parse ' + util.inspect(c), function() {
            var hsl = blend.rgb2hsl(c[0],c[1],c[2]);
            var rgb = blend.hsl2rgb(hsl[0],hsl[1],hsl[2]);
            nearlyEqual(rgb, c, 1);
            var hsl2 = blend.rgb2hsl2(c[0],c[1],c[2]);
            var rgb2 = blend.hsl2rgb2(hsl2[0],hsl2[1],hsl2[2]);
            assert.deepEqual(rgb2,rgb);
            nearlyEqual(rgb2, c, 1);
        });
    });
});
