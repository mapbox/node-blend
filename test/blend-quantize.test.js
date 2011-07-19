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

exports['test blend function with 128 colors'] = function(beforeExit) {
    var completed = false;

    blend(images, { format: 'png', quality: 128 }, function(err, data) {
        if (err) throw err;
        assert.imageEqualsFile(data, 'test/fixture/results/10.png', function(err) {
            completed = true;
            if (err) throw err;
        });
    });

    beforeExit(function() { assert.ok(completed); });
};

exports['test blend function with 64 colors'] = function(beforeExit) {
    var completed = false;

    blend(images, { format: 'png', quality: 64 }, function(err, data) {
        if (err) throw err;
        assert.imageEqualsFile(data, 'test/fixture/results/11.png', function(err) {
            completed = true;
            if (err) throw err;
        });
    });

    beforeExit(function() { assert.ok(completed); });
};

exports['test blend function with 16 colors'] = function(beforeExit) {
    var completed = false;

    blend(images, { format: 'png', quality: 16 }, function(err, data) {
        if (err) throw err;
        assert.imageEqualsFile(data, 'test/fixture/results/12.png', function(err) {
            completed = true;
            if (err) throw err;
        });
    });

    beforeExit(function() { assert.ok(completed); });
};


exports['test blend function with 3 colors'] = function(beforeExit) {
    var completed = false;

    blend(images, { format: 'png', quality: 3 }, function(err, data) {
        if (err) throw err;
        assert.imageEqualsFile(data, 'test/fixture/results/13.png', function(err) {
            completed = true;
            if (err) throw err;
        });
    });

    beforeExit(function() { assert.ok(completed); });
};
