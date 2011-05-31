var assert = require('assert');
var Buffer = require('buffer').Buffer;
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
    blend(true, function(err) {
        completed = true;
        assert.ok(err && /First argument must be an array of Buffers/.test(err.message));
    });
    beforeExit(function() { assert.ok(completed); });
};

exports['test first argument empty'] = function(beforeExit) {
    var completed = false;
    blend([], function(err) {
        completed = true;
        assert.ok(err && /First argument must contain at least one Buffer/.test(err.message));
    });
    beforeExit(function() { assert.ok(completed); });
};

exports['test bogus elements in array'] = function(beforeExit) {
    var completed = false;
    blend([1,2,3], function(err) {
        completed = true;
        assert.ok(err && /All elements must be Buffers/.test(err.message));
    });
    beforeExit(function() { assert.ok(completed); });
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
    });

    beforeExit(function() { assert.ok(completed); });
};

exports['test blend function'] = function(beforeExit) {
    var completed = false;

    blend(images, function(err, data) {
        completed = true;
        if (err) throw err;
        // assert.notDeepEqual(images[4], data);
    });

    beforeExit(function() { assert.ok(completed); });
};
