var assert = require('assert');
var fs = require('fs');

var blend = require('..');
var utilities = require('./support/utilities');


var images = [
    fs.readFileSync('test/fixture/10.jpg'),
    fs.readFileSync('test/fixture/11.jpg')
];

describe('reencode', function() {
    it('should reencode JPEG 10', function(done) {
        blend([ images[0] ], { reencode: true }, function(err, data, warnings) {
            if (err) return done(err);
            assert.deepEqual(warnings, []);
            assert.ok(data.length < images[0].length);
            utilities.imageEqualsFile(data, 'test/fixture/results/test10.jpg', done);
        });
    });

    it('should reencode JPEG 11', function(done) {
        blend([ images[1] ], { reencode: true }, function(err, data, warnings) {
            if (err) return done(err);
            assert.deepEqual(warnings, []);
            assert.ok(data.length < images[1].length);
            utilities.imageEqualsFile(data, 'test/fixture/results/test11.jpg', done);
        });
    });

});
