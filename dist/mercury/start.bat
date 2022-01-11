@echo off
pushd %~dp0
taskkill /f /im amdaemon.exe > nul 2>&1
:LOOP
start inject -d -k mercuryhook.dll amdaemon.exe -f -c config.json config_region_jpn.json config_video_clone.json config_video_clone_flip.json config_video_dual.json config_video_dual_flip.json
inject -d -k mercuryhook.dll ../WindowsNoEditor/Mercury/Binaries/Win64/Mercury-Win64-Shipping.exe
taskkill /f /im amdaemon.exe > nul 2>&1
echo.
echo Game processes have terminated
pause