#!/bin/sh

cd zgw
rm -rf build_prod
mkdir build_prod
cd build_prod
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/debian_stretch_armhf.cmake ..
make -j4
make package
