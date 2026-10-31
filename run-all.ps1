# TerraLink - Run Both Backend and Frontend
Write-Host "=========================================================" -ForegroundColor Cyan
Write-Host "          Starting TerraLink NTN-Sim Full Stack          " -ForegroundColor Cyan
Write-Host "=========================================================" -ForegroundColor Cyan

$backendPath = ".\build\ntn_sim_bin.exe"
if (Test-Path $backendPath) {
    Write-Host "[1/2] Launching NTN-Sim C++ Backend (Port 8080)..." -ForegroundColor Yellow
    Start-Process -FilePath $backendPath -WorkingDirectory ".\ntn-sim"
} else {
    Write-Host "[!] Backend binary not found at $backendPath. Compile with 'cmake --build build' first." -ForegroundColor Red
}

Write-Host "[2/2] Launching Vite React Web UI (Port 5173)..." -ForegroundColor Green
Set-Location ".\web-ui"
npm run dev
