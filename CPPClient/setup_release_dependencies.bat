SET CURRDIR=%~dp0
CD "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build"
CALL vcvars64.bat
CD %CURRDIR%
SET F=%~dp0\socket.io-client-cpp
IF EXIST %F% RMDIR /S /Q %F%
git clone --recurse-submodules https://github.com/socketio/socket.io-client-cpp.git
CD socket.io-client-cpp
cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DOPENSSL_ROOT_DIR="C:\Program Files\OpenSSL-Win64" -DOPENSSL_INCLUDE_DIR="C:\Program Files\OpenSSL-Win64\include" ./
nmake install
cd ..

PAUSE
