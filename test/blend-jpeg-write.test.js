var assert = require('assert');
var Buffer = require('buffer').Buffer;
var crypto = require('crypto');
var fs = require('fs');
var blend = require('..');

function md5(buffer) {
    return crypto.createHash('md5').update(buffer).digest('hex');
}

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
        completed = true;
        if (err) throw err;
        assert.equal('8ca866bba9709dfc05729034cda85909', md5(data));
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
        completed = true;
        if (err) throw err;
        assert.equal('0923ce481b9788469cbf3e067253554a', md5(data));
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
        completed = true;
        if (err) throw err;
        assert.equal('393a40fd34bfd58f4d5c5e658f6f00f1', md5(data));
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
        completed = true;
        if (err) throw err;
        assert.equal('393a40fd34bfd58f4d5c5e658f6f00f1', md5(data));
    });

    beforeExit(function() { assert.ok(completed); });
};

