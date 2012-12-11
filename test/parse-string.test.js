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
    '94eeff': { hue: 192.16510903426794, saturation: 100, y0: 0, y1: 1.5803921568627453 },
    'ff7f00': { hue: 30.29738562091503, saturation: 100, y0: 0, y1: 1 },
    '#4c2d00': { hue: 36.01973684210527, saturation: 100, y0: -0.7019607843137254, y1: 1 }
};

describe('parse tinting string - old method', function() {
    Object.keys(old_mappings).forEach(function(str) {
        it('parse old ' + util.inspect(str), function() {
            var options = blend.parseTintStringOld(str);
            assert.deepEqual(options, old_mappings[str]);
        });
    });
});


var new_mappings = {
    '0.083;0.5;0x1;0x0.5':{ h: [0.083,0.083], s: [0.5,0.5], l: [0,1], a: [0,0.5] },
    '0x1;0x1;0x1;0x1':{ h: [0,1], s: [0,1], l: [0,1], a: [0,1] },
    '.5x0;1x0':{ h: [.5,0], s: [1,0] },
    'ffffff': { h:[0,0],s:[0,0],l:[0,2] },
    'ff7f00': { h: [30.29738562091503/365,30.29738562091503/365], s: [1,1], l:[0,1] },
    '#4c2d00': { h: [36.01973684210527/365,36.01973684210527/365], s: [1,1], l: [-0.7019607843137254,1] }
}

describe('parse tinting string - new method', function() {
    Object.keys(new_mappings).forEach(function(str) {
        it('parse new ' + util.inspect(str), function() {
            var options = blend.parseTintString(str);
            assert.deepEqual(options, new_mappings[str]);
        });
    });
});

var upgradable = {
    '':'',
    '20':'0.0548x0.0548',
    '20;40':'0.0548x0.0548;0.4000x0.4000',
    '30;84;0.5':'0.0822x0.0822;0.8400x0.8400;0.5000x1',
    '30;84;.3;.2':'0.0822x0.0822;0.8400x0.8400;0.3000x0.2000',
    'ffffff': 'ffffff',
    'ff7f00': 'ff7f00',
    '#4c2d00': '#4c2d00'
}

describe('upgrading tint string', function() {
    Object.keys(upgradable).forEach(function(str) {
        it('upgrade ' + util.inspect(str), function() {
            var options = blend.upgradeTintString(str,4);
            assert.deepEqual(options, upgradable[str]);
        });
    });
});
