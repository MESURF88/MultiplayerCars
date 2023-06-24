SET CURRDIR=%~dp0
CD "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build"
CALL vcvars64.bat
cd %CURRDIR%
SET F=%~dp0\curl
IF NOT EXIST %F% git clone https://github.com/curl/curl.git
CD curl
IF NOT EXIST %F%\curl-builddbg MKDIR curl-builddbg
CD curl-builddbg
cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
nmake install
SET PATH=%PATH%;C:\Program Files (x86)\CURL\lib;C:\Program Files (x86)\CURL\include
cd %CURRDIR%
SET F=%~dp0\cpr
IF NOT EXIST %F% git clone https://github.com/libcpr/cpr.git && git checkout a2d35a1cb9f3f7e2f1469d6a189751331dc99f96
CD cpr
IF NOT EXIST %F%\cpr-builddbg MKDIR cpr-builddbg
CD cpr-builddbg
cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
nmake install
cd %CURRDIR%
SET F=%~dp0\nlohmann_json
IF NOT EXIST %F% git clone https://github.com/nlohmann/json.git

PAUSE
