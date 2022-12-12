#!/bin/sh

cd zgw
cd build_native_docker
cmake ..
make all -j4