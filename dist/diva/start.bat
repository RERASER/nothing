@echo off

pushd %~dp0

inject -d -k divahook.dll diva.exe

echo.
echo Game processes have terminated
pause