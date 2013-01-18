var assert = require('assert');
var fs = require('fs');
var path = require('path');
var blend = require('..');
var utilities = require('./support/utilities');

describe('rendering a png image with width 257 (not 256)', function() {
    var options = {
      width: 257,
      height: 256,
      quality:256,
      hextree:true
    };
    it('white image with 257 pixel width', function(done) {
        var file = './test/fixture/tinting/white_full.png';
        var buf = fs.readFileSync(file);
        blend([{buffer:buf}], options, function(err,data) {
            var filepath = './test/width-rounding/' + path.basename(file, '.png') + '-expected.png';
            utilities.imageEqualsFile(data, filepath, done);
        });
    });
});
