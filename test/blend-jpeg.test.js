var assert = require('./support/assert');
var Buffer = require('buffer').Buffer;
var crypto = require('crypto');
var fs = require('fs');
var blend = require('..');

if (process.setMaxListeners) process.setMaxListeners(0);

exports['test blending bogus JPEG image'] = function(beforeExit) {
    var completed = false;
    var buffer = new Buffer(32);
    buffer[0] = 0xFF;
    buffer[1] = 0xD8;
    blend([
        buffer,
        fs.readFileSync('test/fixture/2.png')
    ], function(err, data) {
        completed = true;
        assert.ok(err);
        assert.equal(err.message, "Premature end of JPEG file");
    });

    beforeExit(function() { assert.ok(completed); });
};

exports['test blending a malformed jpeg image'] = function(beforeExit) {
    var completed = false;

    blend([
        fs.readFileSync('test/fixture/1d.jpg'),
        fs.readFileSync('test/fixture/2.png')
    ], function(err, data) {
        completed = true;
        assert.ok(err);
        assert.equal(err.message, "Corrupt JPEG data: bad Huffman code");
    });

    beforeExit(function() { assert.ok(completed); });
};

exports['test blending a jpeg image'] = function(beforeExit) {
    var completed = false;

    blend([
        fs.readFileSync('test/fixture/1a.jpg'),
        fs.readFileSync('test/fixture/2.png')
    ], function(err, data) {
        if (err) throw err;
        assert.imageEqualsFile(data, 'test/fixture/results/3.png', function(err) {
            completed = true;
            if (err) throw err;
        });
    });

    beforeExit(function() { assert.ok(completed); });
};

exports['test blending a grayscale jpeg image'] = function(beforeExit) {
    var completed = false;

    blend([
        fs.readFileSync('test/fixture/1b.jpg'),
        fs.readFileSync('test/fixture/2.png')
    ], function(err, data) {
        if (err) throw err;
        assert.imageEqualsFile(data, 'test/fixture/results/4.png', function(err) {
            completed = true;
            if (err) throw err;
        });
    });

    beforeExit(function() { assert.ok(completed); });
};


exports['test blending a color jpeg image'] = function(beforeExit) {
    var completed = false;

    blend([
        fs.readFileSync('test/fixture/1c.jpg'),
        fs.readFileSync('test/fixture/2.png')
    ], function(err, data) {
        if (err) throw err;
        assert.imageEqualsFile(data, 'test/fixture/results/5.png', function(err) {
            completed = true;
            if (err) throw err;
        });
    });

    beforeExit(function() { assert.ok(completed); });
};

