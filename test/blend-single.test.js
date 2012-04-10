var assert = require('assert');
var fs = require('fs');

var blend = require('..');
var utilities = require('./support/utilities');


var images = [
    fs.readFileSync('test/fixture/1.png'),
    fs.readFileSync('test/fixture/2.png'),
    fs.readFileSync('test/fixture/pattern.png')
];


describe('reencode', function() {
    it('should reencode as PNG 24 bit', function(done) {
        blend([ images[0] ], { reencode: true, compression: 1 }, function(err, data, warnings) {
            if (err) return done(err);
            assert.deepEqual(warnings, []);
            assert.ok(data.length > 16000);
            utilities.imageEqualsFile(data, 'test/fixture/results/14.png', done);
        });
    });

    it('should reencode as PNG 24 bit with a better compression ratio', function(done) {
        blend([ images[0] ], { reencode: true, compression: 9 }, function(err, data, warnings) {
            if (err) return done(err);
            assert.deepEqual(warnings, []);
            assert.ok(data.length <= 16000);
            utilities.imageEqualsFile(data, 'test/fixture/results/14.png', done);
        });
    });

    it('should reencode as JPEG 80%', function(done) {
        blend([ images[0] ], { reencode: true, format: 'jpeg' }, function(err, data, warnings) {
            if (err) return done(err);
            assert.deepEqual(warnings, []);
            assert.ok(data.length < 10000);
            utilities.imageEqualsFile(data, 'test/fixture/results/15.jpg', done);
        });
    });

    it('should reencode as JPEG 40%', function(done) {
        blend([ images[0] ], { reencode: true, format: 'jpeg', quality: 40 }, function(err, data, warnings) {
            if (err) return done(err);
            assert.deepEqual(warnings, []);
            assert.ok(data.length < 5000);
            utilities.imageEqualsFile(data, 'test/fixture/results/16.jpg', done);
        });
    });

    it('should reencode as PNG 32 colors', function(done) {
        blend([ images[0] ], { reencode: true, quality: 32 }, function(err, data, warnings) {
            if (err) return done(err);
            assert.deepEqual(warnings, []);
            assert.ok(data.length < 12000);
            utilities.imageEqualsFile(data, 'test/fixture/results/17.png', done);
        });
    });

    it('should reencode as PNG 8 colors', function(done) {
        blend([ images[0] ], { reencode: true, quality: 8 }, function(err, data, warnings) {
            if (err) return done(err);
            assert.deepEqual(warnings, []);
            assert.ok(data.length < 8000);
            utilities.imageEqualsFile(data, 'test/fixture/results/18.png', done);
        });
    });

    it('should reencode 2 images with the uppermost opaque', function(done) {
        blend([ images[1], images[0] ], { reencode: true }, function(err, data, warnings) {
            if (err) return done(err);
            assert.deepEqual(warnings, []);
            assert.ok(data.length > 15000);
            utilities.imageEqualsFile(data, 'test/fixture/results/14.png', done);
        });
    });

    it('should reencode an interlaced PNG', function(done) {
        blend([ images[2] ], { reencode: true }, function(err, data, warnings) {
            if (err) return done(err);
            assert.deepEqual(warnings, []);
            utilities.imageEqualsFile(data, 'test/fixture/results/26.png', done);
        });
    });
});
