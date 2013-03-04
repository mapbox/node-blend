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


describe('quantization', function() {
    it('should quantize to 128 colors', function(done) {
        blend(images, { format: 'png', quality: 128 }, function(err, data, warnings) {
            if (err) return done(err);
            assert.deepEqual(warnings, []);
            utilities.imageEqualsFile(data, 'test/fixture/results/1.png', 0.01, done);
        });
    });

    it('should quantize to 64 colors', function(done) {
        blend(images, { format: 'png', quality: 64 }, function(err, data, warnings) {
            if (err) return done(err);
            assert.deepEqual(warnings, []);
            utilities.imageEqualsFile(data, 'test/fixture/results/1.png', 0.02, done);
        });
    });

    it('should quantize to 16 colors', function(done) {
        blend(images, { format: 'png', quality: 16 }, function(err, data, warnings) {
            if (err) return done(err);
            assert.deepEqual(warnings, []);
            utilities.imageEqualsFile(data, 'test/fixture/results/1.png', 0.05, done);
        });
    });

    it('should quantize to 8 colors', function(done) {
        blend(images, { format: 'png', quality: 16 }, function(err, data, warnings) {
            if (err) return done(err);
            assert.deepEqual(warnings, []);
            utilities.imageEqualsFile(data, 'test/fixture/results/1.png', 0.05, done);
        });
    });
});
