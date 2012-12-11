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
                var options = {
                  width: 256,
                  height: 256,
                  quality:256,
                  hextree:true
                };
                tint([{buffer:fs.readFileSync('./test/source/' + name + '.png'),
                       tint:new_o}], options, function(err,data) {
                    var filepath = './test/tinted/' + file;
                    utilities.imageEqualsFile(data, filepath, done);
                });
            });
        });
});

var TINTS = [
  "0x1;0x1;0x1;0x1", // no change
  ".5x1;1x1;0x1;0x1", // teal
  "0x1;0x1;0x1;0x.5", // half alpha
  ".1x1;.3x1;0x.9;0x1", // sepia
  "0;54;1;0;1"
]

describe('tinting combinations on varied images', function() {
    fs.readdirSync('./test/fixture/tinting')
        .filter(function(file) {
            return (path.extname(file) === '.png' || path.extname(file) === '.jpeg')
                   && file.indexOf('.result.') == -1; }
        )
        .forEach(function(file) {
            TINTS.forEach(function(tinter) {
                var options = {
                  width: 256,
                  height: 256,
                  quality:256,
                  hextree:true
                };
                it(file + '-' + tinter.toString(), function(done) {
                    var buf = fs.readFileSync('./test/fixture/tinting/' + file);
                    var tint_obj;
                    if (tinter.indexOf('x') > -1) {
                       tint_obj = tint.parseTintString(tinter);
                    } else {
                       tint_obj = tint.parseTintString(tint.upgradeTintString(tinter));
                    }
                    tint([{buffer:buf,tint:tint_obj}], options, function(err,data) {
                        var filepath = './test/tint-varied/' + path.basename(file, '.png') + '-' + tinter + ".png";
                        if (!fs.existsSync(filepath)) {
                            fs.writeFileSync(filepath,data);
                        }
                        utilities.imageEqualsFile(data, filepath, done);
                    });
                });
            });
        });
});
