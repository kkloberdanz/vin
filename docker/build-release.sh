#!/bin/bash

source scl_source enable devtoolset-8

cc --version

cd /work

make clean && make -j WARN_FLAGS="-Wall -Wextra"

mkdir -p /work/dist/vin

cp vin /work/dist/vin/
cp LICENSE.txt /work/dist/vin/

cd /work/dist

zip -r vin.zip vin

mv vin.zip /dist/
