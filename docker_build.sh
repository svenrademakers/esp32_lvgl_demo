#!/bin/bash
docker run --rm --device=/dev/ttyACM0 --privileged --user "$(id -u):$(id -g)" -v $(pwd):/project -w /project -e HOME=/tmp espressif/idf idf.py build
