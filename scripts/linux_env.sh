#!/usr/bin/env bash

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

for cmake_dir in "$repo_root"/.tools/cmake-*-linux-*; do
    if [ -x "$cmake_dir/bin/cmake" ]; then
        export PATH="$cmake_dir/bin:$PATH"
        break
    fi
done

for go_dir in "$repo_root"/.tools/go*.linux-*; do
    if [ -x "$go_dir/bin/go" ]; then
        export PATH="$go_dir/bin:$PATH"
        break
    fi
done
