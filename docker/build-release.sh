#!/bin/bash

set -e

cd /work

make clean && make -j WARN_FLAGS="-Wall -Wextra"

mkdir -p /work/dist/vin

cp vin /work/dist/vin/
cp LICENSE.txt /work/dist/vin/

cd /work/dist

zip -r vin.zip vin

mv vin.zip /dist/
