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
        ], function(err, data, warnings) {
            if (err) return done(err);
            assert.deepEqual(warnings, []);
            assert.deepEqual(images[0], data);
            done();
        });
    });

    it('should offset images properly', function(done) {
        blend([
            { buffer: images[1], x: 20, y: 10 },
            { buffer: images[0], x: -30, y: 90 }
        ], {
            width: 256,
            height: 256
        }, function(err, data, warnings) {
            if (err) return done(err);
            assert.deepEqual(warnings, []);
            utilities.imageEqualsFile(data, 'test/fixture/results/22.png', done);
        });
    });

    it('should render an empty images if everything is out of bounds', function(done) {
        blend([
            { buffer: images[1], x: 200, y: 10 },
            { buffer: images[0], x: -300, y: 90 }
        ], {
            width: 128,
            height: 128
        }, function(err, data, warnings) {
            if (err) return done(err);
            assert.deepEqual(warnings, []);
            utilities.imageEqualsFile(data, 'test/fixture/results/23.png', done);
        });
    });

    it('should error out when the resulting image doesn\'t have a width or height', function(done) {
        blend([
            { buffer: images[1], x: -300, y: -300 },
            { buffer: images[0], x: -300, y: -300 }
        ], function(err) {
            assert.ok(err);
            assert.equal(err.message, 'Image dimensions 0x0 are invalid');
            done();
        });
    });

    it('should only render the RGB matte', function(done) {
        blend([
            { buffer: images[1], x: 200, y: 10 },
            { buffer: images[0], x: -300, y: 90 }
        ], {
            width: 128,
            height: 128,
            matte: 'FF007F',
        }, function(err, data, warnings) {
            if (err) return done(err);
            assert.deepEqual(warnings, []);
            utilities.imageEqualsFile(data, 'test/fixture/results/24.png', done);
        });
    });

    it('should only render the RGBA matte', function(done) {
        blend([
            { buffer: images[1], x: 200, y: 10 },
            { buffer: images[0], x: -300, y: 90 }
        ], {
            width: 128,
            height: 128,
            matte: '12345678',
        }, function(err, data, warnings) {
            if (err) return done(err);
            assert.deepEqual(warnings, []);
            utilities.imageEqualsFile(data, 'test/fixture/results/25.png', done);
        });
    });

    it('should output the RGBA matte even if there are no images', function(done) {
        blend([], {
            width: 128,
            height: 128,
            matte: '12345678'
        }, function(err, data, warnings) {
            if (err) return done(err);
            assert.deepEqual(warnings, []);
            utilities.imageEqualsFile(data, 'test/fixture/results/25.png', done);
        });
    });

    it('should not require image dimensions if there are no images', function() {
        assert.throws(function() {
            blend([], { matte: '12345678' }, function() {});
        }, /Without buffers, you have to specify width and height/);
    });

    it('should nicely stitch the images together', function(done) {
        blend([
            { buffer: fs.readFileSync('test/fixture/5241-12663.png'), x: -43, y: -120 },
            { buffer: fs.readFileSync('test/fixture/5242-12663.png'), x: -43+256, y: -120 },
            { buffer: fs.readFileSync('test/fixture/5243-12663.png'), x: -43+512, y: -120 },
            { buffer: fs.readFileSync('test/fixture/5244-12663.png'), x: -43+768, y: -120 },
            { buffer: fs.readFileSync('test/fixture/5241-12664.png'), x: -43, y: -120+256 },
            { buffer: fs.readFileSync('test/fixture/5242-12664.png'), x: -43+256, y: -120+256 },
            { buffer: fs.readFileSync('test/fixture/5243-12664.png'), x: -43+512, y: -120+256 },
            { buffer: fs.readFileSync('test/fixture/5244-12664.png'), x: -43+768, y: -120+256 },
            { buffer: fs.readFileSync('test/fixture/5241-12665.png'), x: -43, y: -120+512 },
            { buffer: fs.readFileSync('test/fixture/5242-12665.png'), x: -43+256, y: -120+512 },
            { buffer: fs.readFileSync('test/fixture/5243-12665.png'), x: -43+512, y: -120+512 },
            { buffer: fs.readFileSync('test/fixture/5244-12665.png'), x: -43+768, y: -120+512 },
            { buffer: fs.readFileSync('test/fixture/5241-12666.png'), x: -43, y: -120+768 },
            { buffer: fs.readFileSync('test/fixture/5242-12666.png'), x: -43+256, y: -120+768 },
            { buffer: fs.readFileSync('test/fixture/5243-12666.png'), x: -43+512, y: -120+768 },
            { buffer: fs.readFileSync('test/fixture/5244-12666.png'), x: -43+768, y: -120+768 }
        ], {
            width: 700,
            height: 600,
            quality: 64
        }, function(err, data, warnings) {
            if (err) return done(err);
            assert.deepEqual(warnings, []);
            utilities.imageEqualsFile(data, 'test/fixture/results/stitched.png', done);
        });
    });
});
