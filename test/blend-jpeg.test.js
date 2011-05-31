var assert = require('assert');
var Buffer = require('buffer').Buffer;
var crypto = require('crypto');
var fs = require('fs');
var blend = require('..');

function md5(buffer) {
    return crypto.createHash('md5').update(buffer).digest('hex');
}

if (process.setMaxListeners) process.setMaxListeners(0);

exports['test blending bogus JPEG image'] = function(beforeExit) {
    var completed = false;
    var buffer = new Buffer(32);
    buffer[0] = 255;
    buffer[1] = 216;
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
        completed = true;
        if (err) throw err;
        assert.equal('4e43abdd243ccafc80a7da9b0efe3086', md5(data));
    });

    beforeExit(function() { assert.ok(completed); });
};

exports['test blending a grayscale jpeg image'] = function(beforeExit) {
    var completed = false;

    blend([
        fs.readFileSync('test/fixture/1b.jpg'),
        fs.readFileSync('test/fixture/2.png')
    ], function(err, data) {
        completed = true;
        if (err) throw err;
        assert.equal('6a11a65c397f9f3929791703dd61645f', md5(data));
    });

    beforeExit(function() { assert.ok(completed); });
};


exports['test blending a color jpeg image'] = function(beforeExit) {
    var completed = false;

    blend([
        fs.readFileSync('test/fixture/1c.jpg'),
        fs.readFileSync('test/fixture/2.png')
    ], function(err, data) {
        completed = true;
        if (err) throw err;
        assert.equal('393a40fd34bfd58f4d5c5e658f6f00f1', md5(data));
    });

    beforeExit(function() { assert.ok(completed); });
};

