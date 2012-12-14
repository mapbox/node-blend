var fs = require('fs');
var blend = require('..');
var Queue = require('./queue');

// Actual benchmarking code:
var iterations = 200;
var concurrency = 10;

var images = [
    { buffer: fs.readFileSync('test/fixture/tinting/landsat3.jpg'),
      tint:blend.parseTintString('.1x1;0x1;0x1;0x1')
    },
    { buffer: fs.readFileSync('test/fixture/tinting/heat.png'),
      tint:blend.parseTintString('0x1;0x.5;0x.5;0x1')
    },
    { buffer: fs.readFileSync('test/fixture/tinting/borders.png'),
      tint:blend.parseTintString('0x1;0x0;1x1;0x.5')
    },
    { buffer: fs.readFileSync('test/fixture/tinting/overlay.png'),
      tint:blend.parseTintString('.85x1;.5x1;0x1;0x.8')
    }
];

var written = false;

var queue = new Queue(function(i, done) {
    blend(images, {
        width: 256,
        height: 256,
        quality: 256,
        encoder:'libpng',
        mode:'hextree'
    }, function(err, data) {
        if (!written) {
            fs.writeFileSync('./out-tint1.png', data);
            written = true;
        }
        done();
    });
}, concurrency);

queue.on('empty', function() {
    var msec = Date.now() - start;
    console.warn('Iterations: %d', iterations);
    console.warn('Concurrency: %d', concurrency);
    console.warn('Per second: %d', iterations / (msec / 1000));
});

for (var i = 1; i <= iterations; i++) {
    queue.add(i, false);
}

var start = Date.now();
queue.start();
