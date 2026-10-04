#!/bin/sh
set -eu
cd "$(dirname "$0")"
revision=$(cat libopencm3.version)
if [ ! -d lib/libopencm3/.git ]; then
    mkdir -p lib
    git clone https://github.com/libopencm3/libopencm3.git lib/libopencm3
fi
git -C lib/libopencm3 checkout --detach "$revision"
