# node-blend

This module alpha-composites images of the same size and returns the compressed image.

# Usage

```javascript
var blend = require('blend');
var image1; // Contains a compressed PNG image as a buffer.
var image2;
blend([ image1, image2 ], function(err, result) {
    // result contains the blended result image compressed as PNG.
});

blend([ image1, image2 ], {
    format: 'jpeg',
    quality: 90
}, function(err, result) {
    // result contains the blended result image compressed as JPEG.
});
```

# Installation

To compile, this module requires `libjpeg` and `libpng`.

Both /usr and /usr/local with be searched for these dependencies by default.

On Ubuntu, type `sudo apt-get install libjpeg8-dev libpng12-dev` to install them.

Mac OS X ships with png at /usr/X11, which will also be searched.

If you have jpeg or png installed in a custom location you can do:

    ./configure --with-jpeg=/opt/jpeg --with-png=/opt/png
