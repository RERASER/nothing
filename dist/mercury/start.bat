@echo off
pushd %~dp0

taskkill /f /im amdaemon.exe > nul 2>&1

REM USA
REM start inject -d -k mercuryhook.dll amdaemon.exe -f -c config.json config_lan_install_client.json config_lan_install_server.json config_video_clone.json config_video_dual.json config_video_clone_flip.json config_video_dual_flip.json config_region_exp.json config_region_chn.json config_region_usa.json

REM JP
start inject -d -k mercuryhook.dll amdaemon.exe -f -c config.json config_lan_install_client.json config_lan_install_server.json config_video_clone.json config_video_dual.json config_video_clone_flip.json config_video_dual_flip.json config_region_exp.json config_region_chn.json config_region_jpn.json
inject -d -k mercuryhook.dll ../WindowsNoEditor/Mercury/Binaries/Win64/Mercury-Win64-Shipping.exe

taskkill /f /im amdaemon.exe > nul 2>&1

echo Game processes have terminated