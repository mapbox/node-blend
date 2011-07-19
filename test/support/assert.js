var fs = require('fs');
var path = require('path');
var exec = require('child_process').exec;
var spawn = require('child_process').spawn;

var assert = module.exports = exports = require('assert');

assert.imageEqualsFile = function(buffer, file_b, callback) {
    if (!callback) callback = function(err) { if (err) throw err; };
    file_b = path.resolve(file_b);
    var file_a = '/tmp/' + (Math.random() * 1e20) + '.png';
    var err = fs.writeFileSync(file_a, buffer);
    if (err) throw err;

    exec('compare -metric PSNR ' + file_a + ' "' + file_b + '" /dev/null', function(err, stdout, stderr) {
        if (err) {
            // fs.unlinkSync(file_a);
            callback(err);
        } else {
            stderr = stderr.trim();
            if (stderr === 'inf') {
                fs.unlinkSync(file_a);
                callback(null);
            } else {
                var similarity = parseFloat(stderr);
                var err = new Error('Images not equal(' + similarity + '): ' +
                        file_a  + '    ' + file_b);
                err.similarity = similarity;
                callback(err);
            }
        }
    });
};
