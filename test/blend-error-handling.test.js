var assert = require('./support/assert');
var Buffer = require('buffer').Buffer;
var crypto = require('crypto');
var fs = require('fs');
var blend = require('..');

if (process.setMaxListeners) process.setMaxListeners(0);

var images = [
    fs.readFileSync('test/fixture/32.png')
];

exports['test image loading with "Incorrect bKGD chunk index value"'] = function(exit) {
    var completed = false; exit(function() { assert.ok(completed); });

    blend([ images[0] ], { reencode: true }, function(err, data, warnings) {
        if (err) throw err;
        assert.ok(data.length > 1000 && data.length < 1500, 'reencoding bogus image yields implausible size');
        assert.deepEqual(warnings, [ 'Layer 0: Incorrect bKGD chunk index value' ]);

        // Check that we don't reencode the error.
        blend([ data ], { reencode: true }, function(err, data2, warnings) {
            completed = true;
            if (err) throw err;
            assert.deepEqual(warnings, []);
            assert.deepEqual(data, data2);
        });
    });
};
