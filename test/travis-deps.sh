#!/bin/bash

set -e
export CXXFLAGS="$CXXFLAGS -fPIC"
export CFLAGS="$CFLAGS -fPIC"

build_dir=$(pwd)

wget 'http://prdownloads.sourceforge.net/libpng/libpng-1.2.51.tar.gz?download' -O /tmp/libpng-1.2.51.tar.gz
wget 'http://prdownloads.sourceforge.net/libpng/libpng-1.2.51.tar.gz.asc?download' -O /tmp/libpng-1.2.51.tar.gz.asc
gpg --keyserver pgp.mit.edu --recv-keys A16C640F
gpg --verify /tmp/libpng-1.2.51.tar.gz.asc /tmp/libpng-1.2.51.tar.gz

tar xzf /tmp/libpng-1.2.51.tar.gz -C /tmp/
cd /tmp/libpng-1.2.51
./configure --enable-shared --disable-shared --disable-dependency-tracking
make
sudo make install

wget 'http://prdownloads.sourceforge.net/libjpeg-turbo/libjpeg-turbo-1.3.1.tar.gz?download' -O /tmp/libjpeg-turbo-1.3.1.tar.gz
tar xzf /tmp/libjpeg-turbo-1.3.1.tar.gz -C /tmp/
cd /tmp/libjpeg-turbo-1.3.1
./configure --enable-shared --disable-shared --disable-dependency-tracking
make
sudo make install

wget 'https://webp.googlecode.com/files/libwebp-0.4.0.tar.gz' -O /tmp/libwebp-0.4.0.tar.gz
tar xzf /tmp/libwebp-0.4.0.tar.gz -C /tmp/
cd /tmp/libwebp-0.4.0
./configure --enable-shared --disable-shared --disable-dependency-tracking
make
sudo make install

cd $build_dir
