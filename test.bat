@echo off

if "%1"=="node" (
  set ELECTRON_RUN_AS_NODE=1&&set E_APP_PATH=test\resources\app.asar
) else (
  set ELECTRON_RUN_AS_NODE=&&set E_APP_PATH=
)

call npm run build
node pack
test\electron.exe %E_APP_PATH%
