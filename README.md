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
```

# Installation

To compile, this module requires `libjpeg` and `libpng`. They ship with Mac OS X. On Ubuntu, type `sudo apt-get install libjpeg8-dev libpng12-dev` to install them.
