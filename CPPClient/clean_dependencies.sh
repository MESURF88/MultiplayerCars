#!/bin/bash
cd "$(dirname "$0")"
CURRDIR=$(pwd)
if [ -d "$CURRDIR/curl" ]; then
	rm -rf $CURRDIR/curl
fi;
if [ -d "$CURRDIR/cpr" ]; then
	rm -rf $CURRDIR/cpr
fi;
if [ -d "$CURRDIR/nlohmann_json" ]; then
	rm -rf $CURRDIR/nlohmann_json
fi;
if [ -d "$CURRDIR/simdjson" ]; then
	rm -rf $CURRDIR/simdjson
fi;
echo "Complete, Press Any Key to End"
read -p "$*"
