#!/bin/bash

set -e

if (test -z "$CMAKE_CURRENT_BINARY_DIR"); then
    echo "CMAKE_CURRENT_BINARY_DIR is not set"
    exit 1
fi

objcopy $CMAKE_CURRENT_BINARY_DIR/libroot.so --update-section _haiku_revision=<(echo -n hcrev$(git rev-list --count HEAD))