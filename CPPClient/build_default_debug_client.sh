#!/bin/bash
cd "$(dirname "$0")"
CURRDIR=$(pwd)
if [ ! -d "$CURRDIR/builddbg" ]; then
	mkdir builddbg
fi;
cd builddbg
cmake -DCMAKE_BUILD_TYPE=Debug -DDEBUGLOCAL=1 .. && cmake -B .
make
cd ..
cp .env "$CURRDIR"/builddbg
echo "Complete, Press Any Key to End"
read -p "$*"


