var assert = require('assert');
var util = require('util');
var blend = require('..');

var old_mappings = {
    '': {},
    '20': { hue: 20 },
    '20;40': { hue: 20, saturation: 40 },
    '30;84;0.5': { hue: 30, saturation: 84, y0: 0.5 },
    '30;84;.3;.2': { hue: 30, saturation: 84, y0: 0.3, y1: 0.2 },
    'ffffff': { hue: 0, saturation: 0, y0: 0, y1: 2 },
    'ff7f00': { hue: 30.29738562091503, saturation: 100, y0: 0, y1: 1 },
    '#4c2d00': { hue: 36.01973684210527, saturation: 100, y0: -0.7019607843137254, y1: 1 }
};

describe('parse tinting string - old method', function() {
    Object.keys(old_mappings).forEach(function(str) {
        it('parse ' + util.inspect(str), function() {
            var options = blend.parseTintStringOld(str);
            assert.deepEqual(options, old_mappings[str]);
        });
    });
});
