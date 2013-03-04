var assert = require('assert');
var fs = require('fs');

var blend = require('..');
var utilities = require('./support/utilities');


var images = [
    fs.readFileSync('test/fixture/1.png'),
    fs.readFileSync('test/fixture/2.png'),
    fs.readFileSync('test/fixture/3.png'),
    fs.readFileSync('test/fixture/4.png'),
    fs.readFileSync('test/fixture/5.png')
];


describe('PNG blending', function() {
    it('should return the last image if it does not have alpha', function(done) {
        blend([ images[1], images[0] ], function(err, data, warnings) {
            if (err) return done(err);
            assert.deepEqual(warnings, []);
            assert.deepEqual(images[0], data);
            done();
        });
    });

    it('should return the correctly blended file for five valid images', function(done) {
        blend(images, function(err, data, warnings) {
            if (err) return done(err);
            assert.deepEqual(warnings, []);
            utilities.imageEqualsFile(data, 'test/fixture/results/1.png', done);
        });
    });

    it('should return the correctly blended file for two valid images', function(done) {
        blend([ images[2], images[3] ], function(err, data, warnings) {
            if (err) return done(err);
            assert.deepEqual(warnings, []);
            utilities.imageEqualsFile(data, 'test/fixture/results/2.png', done);
        });
    });
});


describe('JPEG writing', function() {
    it('should write a JPEG', function(done) {
        blend([
            fs.readFileSync('test/fixture/1a.jpg'),
            fs.readFileSync('test/fixture/2.png')
        ], {
            format: 'jpeg'
        }, function(err, data, warnings) {
            if (err) return done(err);
            assert.deepEqual(warnings, []);
            utilities.imageEqualsFile(data, 'test/fixture/results/6.jpg', 0.01, done);
        });
    });

    it('should write a JPEG with 50% quality', function(done) {
        blend([
            fs.readFileSync('test/fixture/1a.jpg'),
            fs.readFileSync('test/fixture/2.png')
        ], {
            format: 'jpeg',
            quality: 50
        }, function(err, data, warnings) {
            if (err) return done(err);
            assert.deepEqual(warnings, []);
            utilities.imageEqualsFile(data, 'test/fixture/results/7.jpg', 0.01, done);
        });
    });

    it('should write a PNG that is explicitly marked as such', function(done) {
        blend([
            fs.readFileSync('test/fixture/1c.jpg'),
            fs.readFileSync('test/fixture/2.png')
        ], {
            format: 'png'
        }, function(err, data, warnings) {
            if (err) return done(err);
            assert.deepEqual(warnings, []);
            utilities.imageEqualsFile(data, 'test/fixture/results/8.png', 0.01, done);
        });
    });

    it('should write a PNG with an empty parameters hash', function(done) {
        blend([
            fs.readFileSync('test/fixture/1c.jpg'),
            fs.readFileSync('test/fixture/2.png')
        ], {
            // empty
        }, function(err, data, warnings) {
            if (err) return done(err);
            assert.deepEqual(warnings, []);
            utilities.imageEqualsFile(data, 'test/fixture/results/9.png', 0.01, done);
        });
    });

        it('should blend two JPEG images', function(done) {
        blend([
            fs.readFileSync('test/fixture/1a.jpg'),
            fs.readFileSync('test/fixture/2.png')
        ], function(err, data, warnings) {
            if (err) return done(err);
            assert.deepEqual(warnings, []);
            utilities.imageEqualsFile(data, 'test/fixture/results/3.png', done);
        });
    });

    it('should blend grayscale JPEGs', function(done) {
        blend([
            fs.readFileSync('test/fixture/1b.jpg'),
            fs.readFileSync('test/fixture/2.png')
        ], function(err, data, warnings) {
            if (err) return done(err);
            assert.deepEqual(warnings, []);
            utilities.imageEqualsFile(data, 'test/fixture/results/4.png', done);
        });
    });

    it('should blend color JPEGs', function(done) {
        blend([
            fs.readFileSync('test/fixture/1c.jpg'),
            fs.readFileSync('test/fixture/2.png')
        ], function(err, data, warnings) {
            if (err) return done(err);
            assert.deepEqual(warnings, []);
            utilities.imageEqualsFile(data, 'test/fixture/results/5.png', 0.01, done);
        });
    });

    it('should blend weird PNGs', function(done) {
        blend([
            fs.readFileSync('test/fixture/105-2.png'),
            fs.readFileSync('test/fixture/105-1.png')
        ], function(err, data, warnings) {
            if (err) return done(err);
            assert.deepEqual(warnings, []);
            utilities.imageEqualsFile(data, 'test/fixture/results/105.png', done);
        });
    });
});