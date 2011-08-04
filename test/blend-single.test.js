var assert = require('./support/assert');
var Buffer = require('buffer').Buffer;
var crypto = require('crypto');
var fs = require('fs');
var blend = require('..');

if (process.setMaxListeners) process.setMaxListeners(0);

var images = [
    fs.readFileSync('test/fixture/1.png'),
    fs.readFileSync('test/fixture/2.png')
];

exports['reencode with png 24bit'] = function(exit) {
    var completed = false; exit(function() { assert.ok(completed); });

    blend([ images[0] ], { reencode: true }, function(err, data) {
        completed = true;
        if (err) throw err;
        assert.ok(data.length > 15000);
        assert.imageEqualsFile(data, 'test/fixture/results/14.png', function(err) {
            completed = true;
            if (err) throw err;
        });
    });
};

exports['reencode with jpeg 80'] = function(exit) {
    var completed = false; exit(function() { assert.ok(completed); });

    blend([ images[0] ], { reencode: true, format: 'jpeg' }, function(err, data) {
        completed = true;
        if (err) throw err;
        assert.ok(data.length < 10000);
        assert.imageEqualsFile(data, 'test/fixture/results/15.jpg', function(err) {
            completed = true;
            if (err) throw err;
        });
    });
};

exports['reencode with jpeg 40'] = function(exit) {
    var completed = false; exit(function() { assert.ok(completed); });

    blend([ images[0] ], { reencode: true, format: 'jpeg', quality: 40 }, function(err, data) {
        completed = true;
        if (err) throw err;
        assert.ok(data.length < 5000);
        assert.imageEqualsFile(data, 'test/fixture/results/16.jpg', function(err) {
            completed = true;
            if (err) throw err;
        });
    });
};

exports['reencode with png 32 colors'] = function(exit) {
    var completed = false; exit(function() { assert.ok(completed); });

    blend([ images[0] ], { reencode: true, quality: 32 }, function(err, data) {
        completed = true;
        if (err) throw err;
        assert.ok(data.length < 12000);
        assert.imageEqualsFile(data, 'test/fixture/results/17.png', function(err) {
            completed = true;
            if (err) throw err;
        });
    });
};

exports['reencode with png 8 colors'] = function(exit) {
    var completed = false; exit(function() { assert.ok(completed); });

    blend([ images[0] ], { reencode: true, quality: 8 }, function(err, data) {
        completed = true;
        if (err) throw err;
        assert.ok(data.length < 8000);
        // fs.writeFileSync('test/fixture/results/18.png', data);
        assert.imageEqualsFile(data, 'test/fixture/results/18.png', function(err) {
            completed = true;
            if (err) throw err;
        });
    });
};

exports['reencode with 2 images and uppermost opaque'] = function(exit) {
    var completed = false; exit(function() { assert.ok(completed); });

    blend([ images[1], images[0] ], { reencode: true }, function(err, data) {
        completed = true;
        if (err) throw err;
        assert.ok(data.length > 15000);
        assert.imageEqualsFile(data, 'test/fixture/results/14.png', function(err) {
            completed = true;
            if (err) throw err;
        });
    });
};
