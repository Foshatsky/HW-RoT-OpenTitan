#!/usr/bin/env bash
set -e
cd "$(dirname "$0")"
source ../../tools/venv/bin/activate
make -j$(nproc)
