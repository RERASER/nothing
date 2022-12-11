@echo off

pushd %~dp0

inject -d -k cxbhook.dll Rev_v11.exe

echo.
echo Game processes have terminated
pause