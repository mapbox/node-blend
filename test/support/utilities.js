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

exports.imageEqualsFile = function(buffer, file, callback) {
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
    var compare = spawn('compare', ['-metric', 'PSNR', '-', file, '/dev/null' ]);
    var type = path.extname(file);
    var result = path.join(path.dirname(file), path.basename(file, type) + '.result' + type);

    var error = '';
    compare.stderr.on('data', function(data) {
        error += data.toString();
    });
    compare.on('exit', function(code, signal) {
        if (!code && error.trim() === 'inf') {
            if (existsSync(result)) {
                // clean up old failures
                fs.unlinkSync(result);
            }
            callback(null);
        } else {
            fs.writeFileSync(result, buffer);

            if (code) {
                callback(new Error((error || 'Exited with code ' + code) + ': ' + result));
            } else {
                var similarity = parseFloat(error.trim());
                var err = new Error('Images not equal (' + similarity + '):\n' + result + '\n'+file);
                err.similarity = similarity;
                callback(err);
            }
        }
    });

    compare.stdin.write(buffer);
    compare.stdin.end();
};
