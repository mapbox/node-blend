var fs = require('fs');
var util = require('util');
var path = require('path');
var spawn = require('child_process').spawn;

exports.imageEqualsFile = function(buffer, file, callback) {
    var compare = spawn('compare', ['-metric', 'PSNR', '-', path.resolve(file), '/dev/null' ]);

    var error = '';
    compare.stderr.on('data', function(data) {
        error += data.toString();
    });
    compare.on('exit', function(code, signal) {
        if (code) return callback(new Error(error || 'Exited with code ' + code));
        else if (error.trim() === 'inf') callback(null);
        else {
            var similarity = parseFloat(error.trim());
            var err = new Error('Images not equal (' + similarity + ')');
            err.similarity = similarity;
            callback(err);
        }
    });

    compare.stdin.write(buffer);
    compare.stdin.end();
};
