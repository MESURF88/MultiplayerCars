SET CURRDIR=%~dp0
CD "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build"
CALL vcvars64.bat
CD %CURRDIR%
SET F=%~dp0\socket.io-client-cpp
IF EXIST %F% RMDIR /S /Q %F%
REM git clone --recurse-submodules https://github.com/socketio/socket.io-client-cpp.git
git clone --recurse-submodules https://github.com/ErisExchange/socket.io-client-cpp.git
CD socket.io-client-cpp
cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DSIO_TLS=1 -DBOOST_INCLUDEDIR="C:\Program Files\boost\include" -DBOOST_LIBRARYDIR="C:\Program Files\boost\lib" -DBOOST_VER:STRING="1.82" ./
nmake install


PAUSE
