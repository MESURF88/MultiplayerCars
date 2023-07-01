SET CURRDIR=%~dp0
cd %CURRDIR%
SET F=%~dp0\curl
IF EXIST %F% RMDIR /S /Q %F%
cd %CURRDIR%
SET F=%~dp0\cpr
IF EXIST %F% RMDIR /S /Q %F%
cd %CURRDIR%
SET F=%~dp0\nlohmann_json
IF EXIST %F% RMDIR /S /Q %F%
cd %CURRDIR%
SET F=%~dp0\simdjson
IF EXIST %F% RMDIR /S /Q %F%

PAUSE
