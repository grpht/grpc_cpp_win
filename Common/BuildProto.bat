setlocal enabledelayedexpansion

set "CUR_DIR=%~dp0"
set "PROTO_BUILD_FILE=C:\dev\vcpkg\installed\x64-windows\tools\protobuf\protoc.exe"
set "GRPC_PLUGIN=C:\dev\vcpkg\installed\x64-windows\tools\grpc\grpc_cpp_plugin.exe"
set "READ_PATH=%CUR_DIR%protos/"
set "OUT_PATH=%CUR_DIR%src/models/"

set "PROTO_LIST="

for %%f in ("%READ_PATH%*.proto") do (
  echo %%f
  set "PROTO_LIST=!PROTO_LIST!%%~nxf"
)

START "" "%PROTO_BUILD_FILE%" --proto_path="%READ_PATH%" --grpc_out="%OUT_PATH%" --plugin=protoc-gen-grpc="%GRPC_PLUGIN%" !PROTO_LIST!
START "" "%PROTO_BUILD_FILE%" --proto_path="%READ_PATH%" --cpp_out="%OUT_PATH%" !PROTO_LIST!
IF ERRORLEVEL 1 PAUSE

timeout /t 1 /nobreak

@REM XCOPY /Y /I models "../models"
endlocal