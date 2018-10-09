# Changlog

## 2.0.1

- Updated to mapnik 3.7.0
- Dropped windows support

## 1.4.0

- Updated to mapnik 3.6.2

## 1.3.0

- Updated to mapnik 3.6.0

## 1.2.0

- Updated to mapnik 3.5.0

## 1.0.1

 - Loosen mapnik semver to 3.x.

## 1.0.0

 - Update to node-mapnik@3.0.0. Requires C++11 support.

## 0.11.0

 - Drop C++ addon in favor of using upstream node-mapnik blend API.

## 0.10.1

 - Bump release to dodge npm fail.

## 0.10.0

 - Support for pre-built binaries using node-pre-gyp.

## 0.9.1

 - Fixed potential crash on zero length image.

## 0.9.0

 - Node v0.10.x support (removed waf support)
 - Minor hsl fixes ported from Mapnik 2.3.x: faster fmod and improved rounding/static casts

## 0.8.3

 - Support for OS X 10.9

## 0.8.2

 - Maintain build support for all node v0.8.x versions as waf support erodes

## 0.8.1

 - Fix for OS X build and libjpeg turbo

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
