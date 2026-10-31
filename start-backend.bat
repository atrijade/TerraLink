@echo off
title TerraLink C++ Backend (Port 8080)
echo ========================================================
echo       Starting TerraLink NTN-Sim C++ Backend
echo ========================================================
cd /d "%~dp0ntn-sim"
"..\build\ntn_sim_bin.exe"
pause
