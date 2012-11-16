var assert = require('assert');
var fs = require('fs');
var path = require('path');
var tint = require('..');
var utilities = require('./support/utilities');

describe('tinting', function() {
    fs.readdirSync('./test/tinted')
        .filter(function(file) { return path.extname(file) === '.png' && file.indexOf('.result.') == -1; })
        .forEach(function(file) {
            // Parse parameters from filename.
            var parts = path.basename(file, '.png').split('_');
            var name = parts[0];
            var o = tint.parseTintStringOld(parts[1] || '');
            var new_o = tint.parseTintString(tint.upgradeTintString(parts[1]));
            var testName = path.basename(file) + ' --> ';
            if ('hue' in o) testName += 'hue=' + o.hue + '°';
            if ('saturation' in o) testName += ', saturation=' + o.saturation + '%';
            if ('y0' in o) testName += ', y0=' + o.y0.toFixed(2);
            if ('y1' in o) testName += ', y1=' + o.y1.toFixed(2);
            if ('opacity' in o) testName += ', opacity=' + o.opacity.toFixed(2);

            it(testName, function(done) {
                var source = fs.readFileSync('./test/source/' + name + '.png');
                var options = {
                  width: 256,
                  height: 256,
                  hextree:true,
                  tint: new_o
                };
                tint([source], options, function(err,data) {
                    var filepath = './test/tinted/' + file;
                    utilities.imageEqualsFile(data, filepath, done);
                });
            });
        });
});
