SET CURRDIR=%~dp0
CD "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build"
CALL vcvars64.bat
CD %CURRDIR%
SET F=%~dp0\build
IF EXIST %F% RMDIR /S /Q %F%
MKDIR %F%
CD build
cmake -DSIO_TLS=1 ..
cd ..
PAUSE
