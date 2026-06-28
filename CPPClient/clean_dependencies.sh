#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

rm -rf build builddbg target targetdbg
rm -rf curl cpr json nlohmann_json nholmann_json simdjson

echo "Client build and legacy dependency folders removed."
