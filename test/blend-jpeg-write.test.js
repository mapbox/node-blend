var assert = require('./support/assert');
var Buffer = require('buffer').Buffer;
var crypto = require('crypto');
var fs = require('fs');
var blend = require('..');

if (process.setMaxListeners) process.setMaxListeners(0);

exports['test output with invalid format'] = function(beforeExit) {
    var completed = false;

    assert.throws(function() {
        blend([
            fs.readFileSync('test/fixture/1c.jpg'),
            fs.readFileSync('test/fixture/2.png')
        ], {
            format: 'xbm'
        }, function(err, data) {
            completed = true;
        });
    }, /Invalid output format/);

    beforeExit(function() { assert.ok(!completed); });
};

exports['test output with negative quality'] = function(beforeExit) {
    var completed = false;

    assert.throws(function() {
        blend([
            fs.readFileSync('test/fixture/1c.jpg'),
            fs.readFileSync('test/fixture/2.png')
        ], {
            format: 'jpeg',
            quality: -10
        }, function(err, data) {
            completed = true;
        });
    }, /JPEG quality is range 0-100/);

    beforeExit(function() { assert.ok(!completed); });
};

exports['test output with quality above 100'] = function(beforeExit) {
    var completed = false;

    assert.throws(function() {
        blend([
            fs.readFileSync('test/fixture/1c.jpg'),
            fs.readFileSync('test/fixture/2.png')
        ], {
            format: 'jpeg',
            quality: 110
        }, function(err, data) {
            completed = true;
        });
    }, /JPEG quality is range 0-100/);

    beforeExit(function() { assert.ok(!completed); });
};

exports['test output as jpeg'] = function(beforeExit) {
    var completed = false;

    blend([
        fs.readFileSync('test/fixture/1a.jpg'),
        fs.readFileSync('test/fixture/2.png')
    ], {
        format: 'jpeg'
    }, function(err, data) {
        if (err) throw err;
        assert.imageEqualsFile(data, 'test/fixture/results/6.jpg', function(err) {
            completed = true;
            if (err) throw err;
        });
    });

    beforeExit(function() { assert.ok(completed); });
};

exports['test output as jpeg with different quality'] = function(beforeExit) {
    var completed = false;

    blend([
        fs.readFileSync('test/fixture/1a.jpg'),
        fs.readFileSync('test/fixture/2.png')
    ], {
        format: 'jpeg',
        quality: 50
    }, function(err, data) {
        if (err) throw err;
        assert.imageEqualsFile(data, 'test/fixture/results/7.jpg', function(err) {
            completed = true;
            if (err) throw err;
        });
    });

    beforeExit(function() { assert.ok(completed); });
};

exports['test output as png'] = function(beforeExit) {
    var completed = false;

    blend([
        fs.readFileSync('test/fixture/1c.jpg'),
        fs.readFileSync('test/fixture/2.png')
    ], {
        format: 'png'
    }, function(err, data) {
        if (err) throw err;
        assert.imageEqualsFile(data, 'test/fixture/results/8.png', function(err) {
            completed = true;
            if (err) throw err;
        });
    });

    beforeExit(function() { assert.ok(completed); });
};

exports['test outputting with empty hash'] = function(beforeExit) {
    var completed = false;

    blend([
        fs.readFileSync('test/fixture/1c.jpg'),
        fs.readFileSync('test/fixture/2.png')
    ], {

    }, function(err, data) {
        if (err) throw err;
        assert.imageEqualsFile(data, 'test/fixture/results/9.png', function(err) {
            completed = true;
            if (err) throw err;
        });
    });

    beforeExit(function() { assert.ok(completed); });
};

