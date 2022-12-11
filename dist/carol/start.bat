@echo off

pushd %~dp0

taskkill /f /im aimeReaderHost.exe > nul 2>&1

start /min inject -d -k carolhook.dll aimeReaderHost.exe -p 10
inject -d -k carolhook.dll carol_nu.exe
taskkill /f /im aimeReaderHost.exe > nul 2>&1

echo.
echo Game processes have terminated
pause