var fs = require('fs');
var blend = require('..');
var Queue = require('./queue');

// Actual benchmarking code:
var iterations = 100;
var concurrency = 10;

var images = [
    fs.readFileSync('test/fixture/tinting/iceland.png')
];

var written = false;

var queue = new Queue(function(i, done) {
    blend(images, {
        reencode: true,
        format: 'png',
        quality: 256
    }, function(err, data) {
        if (!written) {
            fs.writeFileSync('./reencode.png', data);
            written = true;
        }
        done();
    });
}, concurrency);

queue.on('empty', function() {
    var msec = Date.now() - start;
    console.warn('[PNG] Iterations: %d', iterations);
    console.warn('[PNG] Concurrency: %d', concurrency);
    console.warn('[PNG] Per second: %d', iterations / (msec / 1000));
});

for (var i = 1; i <= iterations; i++) {
    queue.add(i, false);
}

var start = Date.now();
queue.start();
