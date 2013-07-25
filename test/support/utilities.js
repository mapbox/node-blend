var fs = require('fs');
var util = require('util');
var path = require('path');
var spawn = require('child_process').spawn;
var exec = require('child_process').exec;
var existsSync = require('fs').existsSync || require('path').existsSync

var image_magick_available = true;
var overwrite = false;

exec('compare -h', function(error, stdout, stderr) {
    if (error !== null) {
      image_magick_available = false;
    }
});

exports.imageEqualsFile = function(buffer, file, meanError, callback) {
    if (typeof meanError == 'function') {
        callback = meanError;
        meanError = 0;
    }

    file = path.resolve(file);
    if (overwrite) {
        var err = fs.writeFileSync(file, buffer);
        if (err) {
            err.similarity = 0;
        }
        return callback(err);
    }
    if (!image_magick_available) {
        throw new Error("imagemagick 'compare' tool is not available, please install before running tests");
    }
    var type = path.extname(file);
    var result = path.join(path.dirname(file), path.basename(file, type) + '.result' + type);
    var compare = spawn('compare', ['-metric', 'MAE', result, file, '/dev/null' ]);
    fs.writeFileSync(result, buffer);

    var error = '';
    compare.stderr.on('data', function(data) {
        error += data.toString();
    });
    compare.on('error', function(err) {
        throw err;
    });
    compare.on('exit', function(code, signal) {
        if (code) {
            return callback(new Error((error || 'Exited with code ' + code) + ': ' + result));
        }

        var match = error.match(/^\d+(?:\.\d+)?\s+\((\d+(?:\.\d+)?)\)\s*$/);
        if (!match) {
            throw new Error("could not match similarity string in " + error);
        }
        var similarity = parseFloat(match[1]);
        if (similarity > meanError) {
            var err = new Error('Images not equal: ' + error.trim() + ':\n' + result + '\n'+file);
            err.similarity = similarity;
            callback(err);
        } else {
            if (existsSync(result)) {
                // clean up old failures
                fs.unlinkSync(result);
            }
            callback(null);
        }
    });
    compare.stdin.end();
};
