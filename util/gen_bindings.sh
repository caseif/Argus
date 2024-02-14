#!/usr/bin/env bash

util_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

cd "$util_dir/rustgen"

cargo run
