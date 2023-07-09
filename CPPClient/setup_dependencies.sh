#!/bin/bash
sudo snap install go --classic
sudo apt install perl
sudo apt-get install libssl-dev
sudo apt install curl
sudo apt-get install -y libxinerama-dev
# get cmake 3.20
cd "$(dirname "$0")"
CURRDIR=$(pwd)
cmp=3.20.0
ver=$(cmake --version | head -1 | cut -f3 -d" ")

mapfile -t sorted < <(printf "%s\n" "$ver" "$cmp" | sort -V)

if [[ ${sorted[0]} == "$cmp" ]]; then
    echo "cmake version $ver >= $cmp"
else
	echo "Install cmake dependencies"
	wget https://github.com/Kitware/CMake/releases/download/v3.20.0/cmake-3.20.0.tar.gz
	tar -zvxf cmake-3.20.0.tar.gz
	cd cmake-3.20.0
	./bootstrap
	make -j8
	sudo apt-get install checkinstall
	# this will take more time than you expect
	sudo checkinstall --pkgname=cmake --pkgversion="3.20-custom" --default
	# reset shell cache for tools paths
	hash -r
	sudo rm -rf cmake-3.20.0
	rm cmake-3.20.0.tar.gz
fi
echo "Install boost dependencies"
cd "$CURRDIR"
if [ ! -d "/usr/include/boost" ]; then
	wget https://boostorg.jfrog.io/artifactory/main/release/1.82.0/source/boost_1_82_0.tar.gz
	tar xvf boost_1_82_0.tar.gz
	cd boost_1_82_0
	./bootstrap.sh --prefix=/usr/
	sudo ./b2 install --prefix=/usr/ --with-system --with-date_time --with-random --with-thread --with-chrono link=static runtime-link=shared threading=multi
	cd ..
	rm -rf boost_1_82_0
	rm boost_1_82_0.tar.gz
fi;
echo "Install Release and Debug dependencies"
cd "$CURRDIR"
if [ ! -d "$CURRDIR/curl" ]; then
	git clone https://github.com/curl/curl.git
fi;
cd curl
if [ ! -d "$CURRDIR/curl/curl-build" ]; then
	mkdir curl-build
fi;
cd curl-build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
make
cd ..
if [ ! -d "$CURRDIR/curl/curl-builddbg" ]; then
	mkdir curl-builddbg
fi;
cd curl-builddbg
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
make
cd "$CURRDIR"
if [ ! -d "$CURRDIR/cpr" ]; then
	git clone https://github.com/libcpr/cpr.git
	cd cpr
	git checkout a2d35a1cb9f3f7e2f1469d6a189751331dc99f96
	cd ..
fi;
cd cpr
if [ ! -d "$CURRDIR/cpr/cpr-build" ]; then
	mkdir cpr-build
fi;
cd cpr-build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
sudo make install
cd ..
if [ ! -d "$CURRDIR/cpr/cpr-builddbg" ]; then
	mkdir cpr-builddbg
fi;
cd cpr-builddbg
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
sudo make install
cd "$CURRDIR"
if [ ! -d "$CURRDIR/nholmann_json" ]; then
	git clone https://github.com/nlohmann/json.git
fi;
cd "$CURRDIR"
if [ ! -d "$CURRDIR/simdjson" ]; then
	git clone https://github.com/simdjson/simdjson.git
	cd simdjson
	git checkout v3.2.0
	cd ..
fi;
cd "$CURRDIR"
echo "Complete, Press Any Key to End"
read -p "$*"


