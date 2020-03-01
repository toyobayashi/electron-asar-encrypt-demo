@echo off

call npm run build
node pack
test\electron.exe
