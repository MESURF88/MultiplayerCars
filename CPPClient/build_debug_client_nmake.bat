SET CURRDIR=%~dp0
CD "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build"
CALL vcvars64.bat
CD %CURRDIR%
SET F=%~dp0\nmakebuilddbg
IF EXIST %F% RMDIR /S /Q %F%
MKDIR %F%
CD nmakebuilddbg
cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug .. && cmake -B .
nmake
cd ..
xcopy /y .env %F%
PAUSE
