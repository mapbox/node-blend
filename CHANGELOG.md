# Changlog

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