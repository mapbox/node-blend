var assert = require('assert');
var fs = require('fs');

var blend = require('..');
var utilities = require('./support/utilities');


var images = [
    fs.readFileSync('test/fixture/1.png'),
    fs.readFileSync('test/fixture/2.png'),
    fs.readFileSync('test/fixture/3.png'),
    fs.readFileSync('test/fixture/4.png'),
    fs.readFileSync('test/fixture/5.png')
];


describe('per-image settings', function() {
    it('should accept per-image settings', function(done) {
        blend([
            { buffer: images[1] },
            { buffer: images[0] }
        ], function(err, data) {
            if (err) return done(err);
            assert.deepEqual(images[0], data);
            done();
        });
    });
});
