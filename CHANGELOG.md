# Changlog

## 0.8.0

 - Add support for webp.

## 0.7.1

 - Fix potential memory corruption when writing reduced color png images.

## 0.7.0

 - Modified tinting functionality - now all pixels are converted from rgb to hsl before the tint string is applied and then the pixels are converted back to rgb.
 - Fixed unsigned integer overflow (#27)

## 0.6.0

 - Added HSL tinting support - a tint string can be passed as a property of an image and all rgb pixels will be set as a function of hsl2rgb conversion. See README for details on usage. (#26)
 - Added `--debug` configure option to enable debug builds with assertions

## 0.5.1

 - Fixed possible assert/crash when encoding an image with true white when using dense_hash_map
  
## 0.5.0

 - Switch to using sparsehash/dense_hash_map for performance caching in quantize functions (#24)
 - Added support for MZ_UBER_COMPRESSION when using `encoder:miniz`.

## 0.4.1

 - Added `mode` and `encoder` option to control hextree/octree and libpng/miniz (See README.md for details)
 
## 0.4.0

 - Hextree, fixed palettes, miniz additions

## 0.3.0

 - Node v0.8.x support