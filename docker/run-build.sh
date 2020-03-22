#!/bin/bash

docker build . -t vin-builder
docker run -v $(pwd):/dist --rm vin-builder /work/docker/build-release.sh
