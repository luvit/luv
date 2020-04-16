#!/usr/bin/env bash

# A script for setting up environment for travis-ci testing.
# Sets up the minimum supported Libuv version for testing
# backwards compatibility of Luv

set -eufxo pipefail

LIBUV_VERSION="1.0.0"
LIBUV_BASE="libuv-$LIBUV_VERSION"
INSTALL_DIR="$HOME/.libuv_min"

mkdir "$INSTALL_DIR"

source .ci/platform.sh

curl --silent --location https://github.com/libuv/libuv/archive/v$LIBUV_VERSION.tar.gz | tar xz;
cd $LIBUV_BASE;

sh autogen.sh
./configure --prefix "$INSTALL_DIR"
make
make install
