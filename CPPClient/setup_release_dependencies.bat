SET CURRDIR=%~dp0
CD "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build"
CALL vcvars64.bat
cd %CURRDIR%
SET F=%~dp0\curl
IF NOT EXIST %F% git clone https://github.com/curl/curl.git
CD curl
IF NOT EXIST %F%\curl-build MKDIR curl-build
CD curl-build
cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
nmake install
SET PATH=%PATH%;C:\Program Files (x86)\CURL\lib;C:\Program Files (x86)\CURL\include
cd %CURRDIR%
SET F=%~dp0\cpr
IF NOT EXIST %F% git clone https://github.com/libcpr/cpr.git && CD cpr && git checkout a2d35a1cb9f3f7e2f1469d6a189751331dc99f96 && CD ..
CD cpr
IF NOT EXIST %F%\cpr-build MKDIR cpr-build
CD cpr-build
cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
nmake install
cd %CURRDIR%
SET F=%~dp0\nlohmann_json
IF NOT EXIST %F% git clone https://github.com/nlohmann/json.git
cd %CURRDIR%
SET F=%~dp0\simdjson
IF NOT EXIST %F% git clone https://github.com/simdjson/simdjson.git && CD simdjson && git checkout v3.2.0 && CD ..

PAUSE
