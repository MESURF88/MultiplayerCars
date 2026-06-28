#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"
repo_root="$(cd .. && pwd)"
source "$repo_root/scripts/linux_env.sh"

generator="Unix Makefiles"
if command -v ninja >/dev/null 2>&1; then
    generator="Ninja"
fi

cmake -S . -B build -G "$generator" -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

echo "Release client built at CPPClient/build/carclient"
