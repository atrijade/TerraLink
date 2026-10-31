@echo off
title TerraLink Web UI (Port 5173)
echo ========================================================
echo       Starting TerraLink Cesium 3D Web UI
echo ========================================================
cd /d "%~dp0web-ui"
npm run dev
pause
