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


function checkSimilarity(similarity, done) {
    return function(err) {
        if (err && !err.similarity) return done(err);
        assert.ok(err.similarity >= similarity);
        done();
    }
}

describe('quantization', function() {
    it('should quantize to 128 colors', function(done) {
        blend(images, { format: 'png', quality: 128 }, function(err, data, warnings) {
            if (err) return done(err);
            assert.deepEqual(warnings, []);
            utilities.imageEqualsFile(data, 'test/fixture/results/1.png', checkSimilarity(35, done));
        });
    });

    it('should quantize to 64 colors', function(done) {
        blend(images, { format: 'png', quality: 64 }, function(err, data, warnings) {
            if (err) return done(err);
            assert.deepEqual(warnings, []);
            utilities.imageEqualsFile(data, 'test/fixture/results/1.png', checkSimilarity(30, done));
        });
    });

    it('should quantize to 16 colors', function(done) {
        blend(images, { format: 'png', quality: 16 }, function(err, data, warnings) {
            if (err) return done(err);
            assert.deepEqual(warnings, []);
            utilities.imageEqualsFile(data, 'test/fixture/results/1.png', checkSimilarity(24, done));
        });
    });

    it('should quantize to 8 colors', function(done) {
        blend(images, { format: 'png', quality: 16 }, function(err, data, warnings) {
            if (err) return done(err);
            assert.deepEqual(warnings, []);
            utilities.imageEqualsFile(data, 'test/fixture/results/1.png', checkSimilarity(20, done));
        });
    });
});
