var assert = require('assert');
var fs = require('fs');

var blend = require('..');
var utilities = require('./support/utilities');

// Polyfill buffers.
if (!Buffer) {
    var Buffer = require('buffer').Buffer;
    var SlowBuffer = require('buffer').SlowBuffer;
    SlowBuffer.prototype.fill = Buffer.prototype.fill = function(fill) {
        for (var i = 0; i < this.length; i++) {
            this[i] = fill;
        }
    };
}


var images = [
    fs.readFileSync('test/fixture/1.png'),
    fs.readFileSync('test/fixture/2.png'),
    fs.readFileSync('test/fixture/32.png'),
    fs.readFileSync('test/fixture/1293.jpg'),
    fs.readFileSync('test/fixture/1294.png')
];


describe('invalid arguments', function() {
    it('should throw with a bogus first argument', function(done) {
        blend(true, function(err) {
            assert.ok(err);
            assert.equal('First argument must be an array of Buffers', err.message);
            done();
        });
    });

    it('should throw with an empty first argument', function(done) {
        blend([], function(err) {
            assert.ok(err);
            assert.equal('First argument must contain at least one Buffer', err.message);
            done();
        });
    });

    it('should throw if the first argument contains bogus elements', function(done) {
        blend([1, 2, 3], function(err) {
            assert.ok(err);
            assert.equal('First argument must contain at least one Buffer', err.message);
            done();
        });
    });

    it('should not allow unknown formats', function(done) {
        blend([
            fs.readFileSync('test/fixture/1c.jpg'),
            fs.readFileSync('test/fixture/2.png')
        ], {
            format: 'xbm'
        }, function(err) {
            assert.ok(err);
            assert.equal("Invalid output format", err.message);
            done();
        });
    });

    it('should not allow negative quality', function(done) {
        blend([
            fs.readFileSync('test/fixture/1c.jpg'),
            fs.readFileSync('test/fixture/2.png')
        ], {
            format: 'jpeg',
            quality: -10
        }, function(err) {
            assert.ok(err);
            assert.equal("invalid jpeg quality: '-10'", err.message);
            done();
        });
    });

    it('should not allow quality above 100', function(done) {
        blend([
            fs.readFileSync('test/fixture/1c.jpg'),
            fs.readFileSync('test/fixture/2.png')
        ], {
            format: 'jpeg',
            quality: 110
        }, function(err) {
            assert.ok(err);
            assert.equal("invalid jpeg quality: '110'", err.message);
            done();
        });
    });

    it('should not allow compression level above what zlib supports', function(done) {
        blend([
            fs.readFileSync('test/fixture/1c.jpg'),
            fs.readFileSync('test/fixture/2.png')
        ], {
            compression:10
        }, function(err) {
            assert.ok(err);
            assert.equal("invalid compression value: (only -1 through 9 are valid)", err.message);
            done();
        });
    });

    it('should not allow compression level above what miniz supports', function(done) {
        blend([
            fs.readFileSync('test/fixture/1c.jpg'),
            fs.readFileSync('test/fixture/2.png')
        ], {
            compression:11,
            encoder:'miniz'
        }, function(err) {
            assert.ok(err);
            assert.equal("invalid compression parameter: 11 (only -1 through 10 are valid)", err.message);
            done();
        });
    });

    it('should not allow negative image width', function(done) {
        blend(images, { width: -20, height: 20 }, function(err) {
            assert.ok(err);
            done();
        });
    });

    it('should not allow negative image height', function(done) {
        blend(images, { width: 20, height: -20 }, function(err) {
            assert.ok(err);
            done();
        });
    });

    it('should not allow empty objects', function(done) {
        blend([
            { buffer: images[1] },
            { }
        ], function(err) {
            assert.ok(err);
            assert.equal("All elements must be Buffers or objects with a 'buffer' property", err.message);
            done();
        });
    });

    it('should not allow objects that don\'t have a Buffer', function(done) {
        blend([
            { buffer: images[1] },
            { buffer: false }
        ], function(err) {
            assert.ok(err);
            assert.equal("All elements must be Buffers or objects with a 'buffer' property", err.message);
            done();
        });
    });
});

describe('invalid images', function() {
    it('should report an unknown image format with bogus buffers', function(done) {
        var buffer = new Buffer(1024);
        buffer.fill(0);
        blend([ buffer, buffer ], function(err, data) {
            if (!err) return done(new Error('Error expected'));
            assert.equal(err.message, "Failed to load from buffer");
            done();
        });
    });

    it('should report an invalid chunk type with a semi-bogus buffer', function(done) {
        var buffer = new Buffer('\x89\x50\x4E\x47\x0D\x0A\x1A\x0A' + Array(48).join('\0'), 'binary');
        blend([ buffer, images[1] ], function(err, data) {
            if (!err) return done(new Error('Error expected'));
            assert.equal(err.message, 'failed to read invalid png');
            done();
        });
    });

    it('should report a read error with a buffer that only contains the header', function(done) {
        var buffer = new Buffer('\x89\x50\x4E\x47\x0D\x0A\x1A\x0A', 'binary');
        blend([ buffer, images[1] ], function(err, data) {
            if (!err) return done(new Error('Error expected'));
            assert.equal(err.message, 'failed to read invalid png');
            done();
        });
    });

    // @TODO how to check this chunk independently of mapnik.
    it.skip('should error out with "Incorrect bKGD chunk index value"', function(done) {
        blend([ images[2] ], { reencode: true }, function(err, data, warnings) {
            if (err) return done(err);
            assert.ok(data.length > 600 && data.length < 1200, 'reencoding bogus image yields implausible size');
            assert.deepEqual(warnings, [ 'Layer 0: Incorrect bKGD chunk index value' ]);

            // Check that we don't reencode the error.
            blend([ data ], { reencode: true }, function(err, data2, warnings) {
                if (err) return done(err);
                assert.deepEqual(warnings, []);
                assert.deepEqual(data, data2);
                done();
            });
        });
    });

    it('should report a bogus Huffman table definition', function(done) {
        blend([ images[2], images[3] ], function(err, data, warnings) {
            if (!err) return done(new Error('expected error'));
            assert.equal(err.message, 'JPEG Reader: libjpeg could not read image');

            // Test working state after error.
            blend([ images[2] ], { reencode: true }, done);
        });
    });

    it('should error out on blending an invalid JPEG image', function(done) {
        var buffer = new Buffer(32);
        buffer.fill(0);
        buffer[0] = 0xFF;
        buffer[1] = 0xD8;
        blend([
            buffer,
            fs.readFileSync('test/fixture/2.png')
        ], function(err, data) {
            assert.ok(err);
            assert.ok('JPEG Reader: failed to read header' === err.message ||
                'JPEG Reader: libjpeg could not read image' === err.message);
            done();
        });
    });

    it('should error out on blending a malformed JPEG image', function(done) {
        blend([
            fs.readFileSync('test/fixture/1d.jpg'),
            fs.readFileSync('test/fixture/2.png')
        ], function(err, data) {
            assert.ok(err);
            assert.equal('JPEG Reader: libjpeg could not read image', err.message);
            done();
        });
    });
});
