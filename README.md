# node-blend

This module can re-encode one or more images of the same size. It supports stiching multiple images together into a single image, alpha-compositing, color quantization, and various compression options to produce highly optimized output.

[![Build Status](https://travis-ci.org/mapbox/node-blend.svg)](https://travis-ci.org/mapbox/node-blend)

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

blend([
    { buffer: images[1], x: 20, y: 10 },
    { buffer: images[0], x: -30, y: 90 }
], {
    width: 256,
    height: 256
}, function(err, data) {
    // result contains the blended result image compressed as JPEG.
});
```

### Options

The first argument is an array of either Buffers containing image data, or
Objects with the following potential properties:

- `buffer`: Buffer containing image data
- `x`: image offset in the X dimension
- `y`: image offset in the Y dimension

The second argument is an optional options Object with the following potential
properties:

- `format`: `jpeg`, `png`, or `webp`
- `quality`: integer indicating the quality of the final image. Meaning and range differs per format. For JPEG and webp the range is from 0-100. It defaults to 80. The lower the number the lower image quality and smaller the final image size. For PNG range is from 2-256. It means the # of colors to reduce the image to using. The lower the number the lower image quality and smaller the final image size.
- `width`: integer, default 0: final width of blended image. If options provided with no width value it will default to 0
- `height`: integer, default 0: final width of blended image. If options provided with no height value it will default to 0
- `reencode`: boolean, default false
- `matte`: when alpha is used this is the color to initialize the buffer to (reencode will be set to true automatically when a matte is supplied)
- `compression`: level of compression to use when format is `png`. The higher value indicates higher compression and implies slower encodeing speeds. The lower value indicates faster encoding but larger final images. Default is 6. If the encoder is `libpng` then the valid range is between 1 and 9. If the encoder is `miniz` then the valid range is between 1 and 10. The reason for this difference is that `miniz` has a special "UBER" compression mode that tries to be extremely small at the potential cost of being extremely slow.
- `palette`: pass a blend.Palette object to be used to reduced PNG images to a fixed array of colors
- `mode`: `octree` or `hextree` - the PNG quantization method to use, from Mapnik: https://github.com/mapnik/mapnik/wiki/OutputFormats. Octree only support a few alpha levels, but is faster while Hextree supports many alpha levels.
- `encoder`: `libpng` or `miniz` - the PNG encoder to use. `libpng` is standard while `miniz` is experimental but faster.

# Installation

    npm install @mapbox/blend@latest

# Development

To run tests for this module, run `npm install --dev` to install the testing framework, then
`npm test`. Tests require [Imagemagick](http://www.imagemagick.org/script/index.php) for its `compare` utility.
