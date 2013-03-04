var fs = require('fs');
var blend = require('..');
var Queue = require('./queue');

// Actual benchmarking code:
var iterations = 1000;
var concurrency = 10;

var images = [
    fs.readFileSync('test/fixture/tinting/iceland.webp')
];

var written = false;

var queue = new Queue(function(i, done) {
    blend(images, {
        reencode: true,
        format: 'jpg'
    }, function(err, data) {
        if (!written) {
            fs.writeFileSync('./reencode.jpg', data);
            written = true;
        }
        done();
    });
}, concurrency);

queue.on('empty', function() {
    var msec = Date.now() - start;
    console.warn('[JPEG] Iterations: %d', iterations);
    console.warn('[JPEG] Concurrency: %d', concurrency);
    console.warn('[JPEG] Per second: %d', iterations / (msec / 1000));
});

for (var i = 1; i <= iterations; i++) {
    queue.add(i, false);
}

var start = Date.now();
queue.start();
