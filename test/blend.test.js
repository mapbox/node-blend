var assert = require('./support/assert');
var Buffer = require('buffer').Buffer;
var crypto = require('crypto');
var fs = require('fs');
var blend = require('..');

if (process.setMaxListeners) process.setMaxListeners(0);

var images = [
    fs.readFileSync('test/fixture/1.png'),
    fs.readFileSync('test/fixture/2.png'),
    fs.readFileSync('test/fixture/3.png'),
    fs.readFileSync('test/fixture/4.png'),
    fs.readFileSync('test/fixture/5.png')
];

exports['test first argument bogus'] = function(beforeExit) {
    var completed = false;

    assert.throws(function() {
        blend(true, function(err) { completed = true; });
    }, /First argument must be an array of Buffers/);

    beforeExit(function() { assert.ok(!completed); });
};

exports['test first argument empty'] = function(beforeExit) {
    var completed = false;

    assert.throws(function() {
        blend([], function(err) { completed = true; });
    }, /First argument must contain at least one Buffer/);

    beforeExit(function() { assert.ok(!completed); });
};

exports['test bogus elements in array'] = function(beforeExit) {
    var completed = false;

    assert.throws(function() {
        blend([1,2,3], function(err) { completed = true; });
    }, /All elements must be Buffers/);

    beforeExit(function() { assert.ok(!completed); });
};

exports['test blend function with topmost image having no alpha'] = function(beforeExit) {
    var completed = false;
    blend([ images[1], images[0] ], function(err, data) {
        completed = true;
        if (err) throw err;
        assert.deepEqual(images[0], data);
    });

    beforeExit(function() { assert.ok(completed); });
};

exports['test blend function with bogus buffer'] = function(beforeExit) {
    var completed = false;
    blend([ new Buffer(1024), new Buffer(1024) ], function(err, data) {
        completed = true;
        assert.ok(err);
        assert.equal(err.message, "Unknown image format");
    });

    beforeExit(function() { assert.ok(completed); });
};

exports['test blend function with semi-bogus buffer'] = function(beforeExit) {
    var completed = false;
    var buffer = new Buffer('\x89\x50\x4E\x47\x0D\x0A\x1A\x0A' + Array(48).join('\0'), 'binary');
    blend([ buffer, images[1] ], function(err, data) {
        completed = true;
        assert.ok(err);
        assert.equal(err.message, '[00][00][00][00]: invalid chunk type');
    });

    beforeExit(function() { assert.ok(completed); });
};

exports['test blend function with buffer that only contains the header'] = function(beforeExit) {
    var completed = false;
    var buffer = new Buffer('\x89\x50\x4E\x47\x0D\x0A\x1A\x0A', 'binary');
    blend([ buffer, images[1] ], function(err, data) {
        completed = true;
        assert.ok(err);
        assert.equal(err.message, 'Read Error');
    });

    beforeExit(function() { assert.ok(completed); });
};

exports['test blend function'] = function(beforeExit) {
    var completed = false;

    blend(images, function(err, data) {
        if (err) throw err;
        assert.imageEqualsFile(data, 'test/fixture/results/1.png', function(err) {
            completed = true;
            if (err) throw err;
        });
    });

    beforeExit(function() { assert.ok(completed); });
};

exports['test blend function 2'] = function(beforeExit) {
    var completed = false;

    blend([ images[2], images[3] ], function(err, data) {
        if (err) throw err;
        assert.imageEqualsFile(data, 'test/fixture/results/2.png', function(err) {
            completed = true;
            if (err) throw err;
        });
    });

    beforeExit(function() { assert.ok(completed); });
};
