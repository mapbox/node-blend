var assert = require('assert');
var Buffer = require('buffer').Buffer;
var fs = require('fs');
var blend = require('..');

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

exports['test blending a jpeg image'] = function(beforeExit) {
    var completed = false;

    blend([
        fs.readFileSync('test/fixture/1a.jpg'),
        fs.readFileSync('test/fixture/2.png')
    ], function(err, data) {
        completed = true;
        if (err) throw err;
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
    });

    beforeExit(function() { assert.ok(completed); });
};

