#!/bin/bash
cd "$(dirname "$0")"
CURRDIR=$(pwd)
if [ ! -d "$CURRDIR/build" ]; then
	mkdir build
fi;
cd build
cmake -DCMAKE_BUILD_TYPE=Release .. && cmake -B .
make
cd ..
cp .env "$CURRDIR"/build
echo "Complete, Press Any Key to End"
read -p "$*"


