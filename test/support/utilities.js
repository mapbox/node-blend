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

    var fixturesize = fs.statSync(file).size;
    var sizediff = Math.abs(fixturesize - buffer.length) / fixturesize;

    var prefix = String(count++)+"-";
    var actualTmp = path.join("/tmp/node-blend",prefix+path.basename(file.replace(path.extname(file),'.actual'+path.extname(file))));
    var resultFile = path.join("/tmp/node-blend",prefix+path.basename(file.replace(path.extname(file),'.result'+path.extname(file))));

    mkdirp.sync(path.dirname(resultFile));
    mkdirp.sync(path.dirname(actualTmp));
    var resultImage = new mapnik.Image.fromBytesSync(buffer);
    var expectImage = new mapnik.Image.open(file);

    if (sizediff > meanError) {
        resultImage.save(resultFile.replace('jpg','jpeg'));
        if (process.env.UPDATE) {
            resultImage.save(file.replace('jpg','jpeg'));
        }
        expectImage.save(actualTmp.replace('jpg','jpeg'));
        return callback(new Error('Image size is too different from fixture: ' + buffer.length + ' vs. ' + fixturesize + '\n\tSee result at ' + resultFile));
    }
    var pxDiff = expectImage.compare(resultImage);

    // Allow < 2% of pixels to vary by > default comparison threshold of 16.
    var pxThresh = resultImage.width() * resultImage.height() * 0.02;

    if (pxDiff > pxThresh) {
        resultImage.save(resultFile.replace('jpg','jpeg'));
        if (process.env.UPDATE) {
            resultImage.save(file.replace('jpg','jpeg'));
        }
        expectImage.save(actualTmp.replace('jpg','jpeg'));
        callback(new Error('Image is too different from fixture: ' + pxDiff + ' pixels > ' + pxThresh + ' pixels\n\tSee result at ' + resultFile));
    } else {
        callback();
    }
}

module.exports.imageEqualsFile = imageEqualsFile;