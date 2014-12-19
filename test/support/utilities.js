var fs = require('fs');
var util = require('util');
var path = require('path');
var spawn = require('child_process').spawn;
var exec = require('child_process').exec;
var existsSync = require('fs').existsSync || require('path').existsSync;
var mapnik = require('mapnik');
var mkdirp = require('mkdirp');
var rimraf = require('rimraf')

//try {
    rimraf.sync("/tmp/node-blend");
//} catch (err) {}

var count = 0;

function imageEqualsFile(buffer, file, meanError, callback) {
    if (typeof meanError == 'function') {
        callback = meanError;
        meanError = 0.05;
    }

    var prefix = String(count++)+"-";
    var ext = path.extname(file);
    var expected_copy = path.join("/tmp/node-blend",prefix+path.basename(file.replace(ext,'.expected'+ext)));
    var resultFile = path.join("/tmp/node-blend",prefix+path.basename(file.replace(ext,'.result'+ext)));

    mkdirp.sync(path.dirname(resultFile));
    mkdirp.sync(path.dirname(expected_copy));

    if (!existsSync(file) || process.env.UPDATE) {
        fs.writeFileSync(file,buffer);
    }

    var resultImage = new mapnik.Image.fromBytesSync(buffer);
    var expectImage = new mapnik.Image.open(file);

    var fixturesize = fs.statSync(file).size;
    var sizediff = Math.abs(fixturesize - buffer.length) / fixturesize;

    if (sizediff > meanError) {
        fs.writeFileSync(resultFile,buffer);
        fs.writeFileSync(expected_copy, fs.readFileSync(file));
        return callback(new Error('Image size is too different from fixture: ' + buffer.length + ' vs. ' + fixturesize + '\n\tSee result at ' + resultFile));
    }
    var pxDiff = expectImage.compare(resultImage);

    // Allow < 2% of pixels to vary by > default comparison threshold of 16.
    var pxThresh = resultImage.width() * resultImage.height() * 0.02;

    if (pxDiff > pxThresh) {
        fs.writeFileSync(resultFile,buffer);
        fs.writeFileSync(expected_copy, fs.readFileSync(file))
        callback(new Error('Image is too different from fixture: ' + pxDiff + ' pixels > ' + pxThresh + ' pixels\n\tSee result at ' + resultFile));
    } else {
        callback();
    }
}

module.exports.imageEqualsFile = imageEqualsFile;