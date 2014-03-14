var a = require('fs').readFileSync(__dirname + '/fixture/segfault-a.png');
var b = require('fs').readFileSync(__dirname + '/fixture/segfault-b.png');
var c = require('fs').readFileSync(__dirname + '/fixture/8.png');
var blend = require('../index.js');
var assert = require('assert');

describe('blend', function() {
    it('should error (a + c)', function(done) {
        blend([a, c], function(err, data) {
            assert.ok(err);
        });
    });
    it('should error (a + b)', function(done) {
        blend([a, b], function(err, data) {
            assert.ok(err);
        });
    });
});
