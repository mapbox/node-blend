#!/bin/bash

set -e
set -o pipefail

export CXXFLAGS="$CXXFLAGS -fPIC"
export CFLAGS="$CFLAGS -fPIC"

build_dir=$(pwd)
if [ "$platform" == "linux" ]; then
    getmd5="md5sum"
else
    getmd5="md5"
fi

# md5sum for libpng from sourceforge UI
wget 'http://prdownloads.sourceforge.net/libpng/libpng-1.2.51.tar.gz?download' -O /tmp/libpng-1.2.51.tar.gz
$getmd5 /tmp/libpng-1.2.51.tar.gz | grep e358f9a265f2063b36f10dc454ee0e17
tar xzf /tmp/libpng-1.2.51.tar.gz -C /tmp/
cd /tmp/libpng-1.2.51
./configure --enable-shared --disable-shared --disable-dependency-tracking
make
sudo make install

# md5sum for libjpeg-turbo from sourceforge UI
wget 'http://prdownloads.sourceforge.net/libjpeg-turbo/libjpeg-turbo-1.3.1.tar.gz?download' -O /tmp/libjpeg-turbo-1.3.1.tar.gz
$getmd5 /tmp/libjpeg-turbo-1.3.1.tar.gz | grep 2c3a68129dac443a72815ff5bb374b05
tar xzf /tmp/libjpeg-turbo-1.3.1.tar.gz -C /tmp/
cd /tmp/libjpeg-turbo-1.3.1
if [ "$platform" == "linux" ]; then
    ./configure --enable-shared --disable-shared --disable-dependency-tracking --with-jpeg8
else
    ./configure --enable-shared --disable-shared --disable-dependency-tracking --with-jpeg8 --host x86_64-apple-darwin
fi
make
sudo make install prefix=/usr/local libdir=/usr/local/lib

wget 'https://webp.googlecode.com/files/libwebp-0.4.0.tar.gz' -O /tmp/libwebp-0.4.0.tar.gz
tar xzf /tmp/libwebp-0.4.0.tar.gz -C /tmp/
cd /tmp/libwebp-0.4.0
./configure --enable-shared --disable-shared --disable-dependency-tracking
make
sudo make install

# ImageMagick is built from scratch on osx as brew installs libjpeg which
# conflicts with libjpeg-turbo.
if [ "$platform" == "darwin" ]; then
    wget 'http://www.imagemagick.org/download/ImageMagick-6.8.8-10.tar.gz' -O /tmp/ImageMagick-6.8.8-10.tar.gz
    tar xzf /tmp/ImageMagick-6.8.8-10.tar.gz -C /tmp/
    cd /tmp/ImageMagick-6.8.8-10
    ./configure
    make
    sudo make install
fi

cd $build_dir
