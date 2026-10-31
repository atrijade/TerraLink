# TerraLink 5G NTN-Sim Web UI

Aerospace-grade 3D visualization canvas and real-time telemetry HUD for **NTN-Sim** (5G Non-Terrestrial Network Discrete-Event Simulator).

---

## Features

* **3D Orbital Earth (CesiumJS)**:
  * Full WGS84 3D Earth globe with dark aerospace theme.
  * LEO Constellation visualization (Walker constellation at ~600 km altitude).
  * 4 orbital plane ribbons with atmospheric glow.
  * Ground Gateways (Svalbard, Frankfurt, Hawaii, Singapore) and User Terminals (UEs).
* **Multi-Layer Dynamic Beams**:
  * **ISL Links (Inter-Satellite)**: Glowing cyan polylines representing active cross-links.
  * **Failed / Severed Links**: Red dashed/pulsing polylines indicating injected outages.
  * **Feeder Links**: Solid gold/amber beams connecting ground gateways to connected satellites.
  * **Access Links**: Dashed blue lines connecting ground users to serving satellites.
* **Real-time 3GPP NTN KPI HUD**:
  * Packet Delivery Ratio (PDR %) with color-coded threshold status (`OPTIMAL`, `DEGRADED`, `CRITICAL`).
  * End-to-end Latency (Mean & 95th Percentile) and Jitter ($\pm\text{ms}$).
  * Mesh Throughput (Mbps) and dropped packet tallies (Queue overflow, TTL expired, No-route).
  * Handover success rate, ping-pong frequency, Doppler shift, and mean SINR.
* **Rolling Telemetry Charts (Recharts)**:
  * Sliding-window graphs for Latency (ms) and Throughput (Mbps).
* **Interactive Fault Injection**:
  * Interactive control panel to select two satellites and click **"Sever Link"** or **"Restore Link"**.
  * Quick select from active ISL links.
  * "Simulate Random Outage" button.
  * Quick "Restore All" button.
  * Telemetry polling rate slider (250ms, 500ms, 1s, 2s).
* **Automatic Fallback Mock Engine**:
  * Allows frontend development and demonstrations even when the C++ backend is offline.

---

## Quick Start

### 1. Install Dependencies
```bash
cd web-ui
npm install
```

### 2. Start the Web UI
```bash
npm run dev
```
Open [http://localhost:5173](http://localhost:5173) in your browser.

### 3. Connect to C++ NTN-Sim Backend (Optional)
To stream live simulation telemetry from the C++ backend:
```bash
# In the root repository
mkdir build && cd build
cmake ..
cmake --build .
./ntn-sim/ntn_sim_bin
```
The REST API runs on `http://localhost:8080`, and the Vite dev server automatically proxies API requests (`/satellites`, `/topology`, `/metrics`, etc.) to the backend.
