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

### Options

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

To compile, this module requires `libjpeg`, `libpng`, and `libwebp`.

Both /usr and /usr/local with be searched for these dependencies by default.

On Ubuntu, type `sudo apt-get install libjpeg8-dev libpng12-dev libwebp-dev` to install them.

Mac OS X ships with png at /usr/X11, which will also be searched.

If you have jpeg or png installed in a custom location you can do:

    ./configure --with-jpeg=/opt/jpeg --with-png=/opt/png

# Development

To run tests for this module, run `npm install --dev` to install the testing framework, then
`npm test`. Tests require [Imagemagick](http://www.imagemagick.org/script/index.php) for its `compare` utility.
