import React from 'react';
import {
  X,
  Power,
  RotateCcw,
  Gauge,
  Cpu,
  Layers,
  LayoutDashboard,
  Eye,
  EyeOff,
  Globe,
  Radio,
  Share2,
  Tag,
  Satellite as SatIcon,
  CheckSquare,
  Square,
  Zap,
} from 'lucide-react';
import type { GlobeLayerState } from '../globe/GlobeControls';

interface CustomizationPanelProps {
  isOpen: boolean;
  onClose: () => void;
  // Simulation Controls
  isSimRunning: boolean;
  onToggleSimulation: () => void;
  speedMultiplier: number;
  onSetSpeedMultiplier: (speed: number) => void;
  onResetSimulation: () => void;
  // Source
  isMockMode: boolean;
  onSetMockMode: (mock: boolean) => void;
  isBackendOnline: boolean;
  // Polling
  pollIntervalMs: number;
  onSetPollInterval: (ms: number) => void;
  // 3D Layers
  globeLayers: GlobeLayerState;
  onToggleGlobeLayer: (key: keyof GlobeLayerState) => void;
  onSetAllGlobeLayers: (enable: boolean) => void;
  // Dashboard Widgets
  showKpis: boolean;
  onToggleKpis: () => void;
  showCharts: boolean;
  onToggleCharts: () => void;
}

export const CustomizationPanel: React.FC<CustomizationPanelProps> = ({
  isOpen,
  onClose,
  isSimRunning,
  onToggleSimulation,
  speedMultiplier,
  onSetSpeedMultiplier,
  onResetSimulation,
  isMockMode,
  onSetMockMode,
  isBackendOnline,
  pollIntervalMs,
  onSetPollInterval,
  globeLayers,
  onToggleGlobeLayer,
  onSetAllGlobeLayers,
  showKpis,
  onToggleKpis,
  showCharts,
  onToggleCharts,
}) => {
  if (!isOpen) return null;

  const speedOptions = [0.5, 1.0, 2.0, 5.0];

  return (
    <div className="fixed inset-y-0 right-0 z-30 w-96 bg-slate-900/95 backdrop-blur-xl border-l border-slate-700/80 shadow-2xl flex flex-col animate-in slide-in-from-right duration-200 text-slate-200">
      {/* Header */}
      <div className="flex items-center justify-between px-5 py-4 border-b border-slate-800 bg-slate-950/40">
        <div className="flex items-center gap-2">
          <div className="p-1.5 rounded-lg bg-cyan-950 text-cyan-400 border border-cyan-800/50">
            <Zap size={16} />
          </div>
          <div>
            <h2 className="text-sm font-bold tracking-wide text-white">
              UI & Simulation Customization
            </h2>
            <p className="text-[11px] text-slate-400">
              Control simulation runtime & visual presentation
            </p>
          </div>
        </div>
        <button
          onClick={onClose}
          className="text-slate-400 hover:text-white p-1 rounded-md hover:bg-slate-800 transition cursor-pointer"
        >
          <X size={18} />
        </button>
      </div>

      {/* Body: Scrollable Settings */}
      <div className="flex-1 overflow-y-auto px-5 py-4 space-y-6 text-xs">
        {/* 1. MASTER SIMULATION TOGGLE */}
        <section className="bg-slate-950/60 rounded-xl p-3.5 border border-slate-800/80 space-y-3">
          <div className="flex items-center justify-between">
            <span className="font-semibold text-slate-200 uppercase tracking-wider text-[11px] flex items-center gap-1.5">
              <Power size={14} className={isSimRunning ? 'text-emerald-400' : 'text-rose-400'} />
              Simulation Runtime
            </span>
            <span
              className={`px-2 py-0.5 rounded text-[10px] font-bold tracking-wider ${
                isSimRunning
                  ? 'bg-emerald-950/80 text-emerald-400 border border-emerald-800/60'
                  : 'bg-rose-950/80 text-rose-400 border border-rose-800/60'
              }`}
            >
              {isSimRunning ? 'RUNNING' : 'STOPPED'}
            </span>
          </div>

          <div className="flex gap-2">
            <button
              onClick={onToggleSimulation}
              className={`flex-1 py-2.5 px-3 rounded-lg font-bold flex items-center justify-center gap-2 transition cursor-pointer ${
                isSimRunning
                  ? 'bg-rose-600 hover:bg-rose-500 text-white shadow-lg shadow-rose-900/40'
                  : 'bg-emerald-600 hover:bg-emerald-500 text-white shadow-lg shadow-emerald-900/40 animate-pulse'
              }`}
            >
              <Power size={14} />
              <span>{isSimRunning ? 'Turn OFF Simulation' : 'Turn ON Simulation'}</span>
            </button>

            <button
              onClick={onResetSimulation}
              title="Reset Simulation Clock"
              className="px-3 py-2.5 bg-slate-800 hover:bg-slate-700 text-slate-300 hover:text-white rounded-lg border border-slate-700/60 transition cursor-pointer"
            >
              <RotateCcw size={14} />
            </button>
          </div>
          <p className="text-[10px] text-slate-400 leading-relaxed">
            Turning off freezes satellite orbital physics, packet transmission, and telemetry streams.
          </p>
        </section>

        {/* 2. SIMULATION SPEED MULTIPLIER */}
        <section className="space-y-2.5">
          <div className="flex items-center justify-between text-slate-300">
            <span className="font-semibold text-[11px] uppercase tracking-wider flex items-center gap-1.5">
              <Gauge size={14} className="text-cyan-400" />
              Orbital Speed Multiplier
            </span>
            <span className="font-mono text-cyan-300 font-bold">{speedMultiplier}x</span>
          </div>

          <div className="grid grid-cols-4 gap-1.5">
            {speedOptions.map((speed) => (
              <button
                key={speed}
                onClick={() => onSetSpeedMultiplier(speed)}
                className={`py-1.5 rounded-lg border text-xs font-mono font-medium transition cursor-pointer ${
                  speedMultiplier === speed
                    ? 'bg-cyan-950 text-cyan-300 border-cyan-600 font-bold shadow-md shadow-cyan-950/60'
                    : 'bg-slate-800/70 text-slate-400 border-slate-700/60 hover:text-slate-200 hover:bg-slate-800'
                }`}
              >
                {speed}x
              </button>
            ))}
          </div>
        </section>

        {/* 3. SIMULATION ENGINE SOURCE */}
        <section className="space-y-2.5">
          <div className="flex items-center justify-between text-slate-300">
            <span className="font-semibold text-[11px] uppercase tracking-wider flex items-center gap-1.5">
              <Cpu size={14} className="text-purple-400" />
              Engine Source
            </span>
            <span
              className={`px-1.5 py-0.5 rounded text-[10px] font-mono ${
                isBackendOnline
                  ? 'bg-emerald-950 text-emerald-300 border border-emerald-800'
                  : 'bg-amber-950 text-amber-300 border border-amber-800'
              }`}
            >
              {isBackendOnline ? 'C++ Active' : 'Fallback Engine'}
            </span>
          </div>

          <div className="grid grid-cols-2 gap-2">
            <button
              onClick={() => onSetMockMode(false)}
              className={`p-2.5 rounded-lg border text-left transition cursor-pointer ${
                !isMockMode
                  ? 'bg-cyan-950/70 border-cyan-600 text-cyan-200'
                  : 'bg-slate-800/50 border-slate-700/60 text-slate-400 hover:text-slate-300'
              }`}
            >
              <div className="font-semibold text-[11px]">C++ Backend</div>
              <div className="text-[10px] text-slate-400">Port 8080 (Live Server)</div>
            </button>

            <button
              onClick={() => onSetMockMode(true)}
              className={`p-2.5 rounded-lg border text-left transition cursor-pointer ${
                isMockMode
                  ? 'bg-purple-950/70 border-purple-600 text-purple-200'
                  : 'bg-slate-800/50 border-slate-700/60 text-slate-400 hover:text-slate-300'
              }`}
            >
              <div className="font-semibold text-[11px]">Browser Physics</div>
              <div className="text-[10px] text-slate-400">Offline Walker Engine</div>
            </button>
          </div>
        </section>

        {/* 4. POLLING REFRESH RATE */}
        <section className="space-y-2">
          <div className="flex justify-between items-center text-slate-300">
            <span className="font-semibold text-[11px] uppercase tracking-wider">
              HUD Refresh Rate
            </span>
            <span className="font-mono text-cyan-300">{pollIntervalMs} ms</span>
          </div>
          <input
            type="range"
            min={200}
            max={2000}
            step={50}
            value={pollIntervalMs}
            onChange={(e) => onSetPollInterval(Number(e.target.value))}
            className="w-full h-1.5 bg-slate-800 rounded-lg appearance-none cursor-pointer accent-cyan-400"
          />
          <div className="flex justify-between text-[10px] text-slate-500">
            <span>200ms (High FPS)</span>
            <span>2000ms (Low Power)</span>
          </div>
        </section>

        {/* 5. 3D GLOBE LAYER VISIBILITY */}
        <section className="space-y-2.5">
          <div className="flex items-center justify-between text-slate-300">
            <span className="font-semibold text-[11px] uppercase tracking-wider flex items-center gap-1.5">
              <Layers size={14} className="text-cyan-400" />
              3D Globe Visual Layers
            </span>
            <div className="flex gap-2">
              <button
                onClick={() => onSetAllGlobeLayers(true)}
                className="text-[10px] text-cyan-400 hover:underline cursor-pointer"
              >
                All ON
              </button>
              <span className="text-slate-600">|</span>
              <button
                onClick={() => onSetAllGlobeLayers(false)}
                className="text-[10px] text-slate-400 hover:underline cursor-pointer"
              >
                All OFF
              </button>
            </div>
          </div>

          <div className="space-y-1.5 bg-slate-950/40 p-2.5 rounded-xl border border-slate-800">
            {/* Satellites */}
            <div
              onClick={() => onToggleGlobeLayer('showSatellites')}
              className="flex items-center justify-between p-2 rounded-lg hover:bg-slate-800/60 transition cursor-pointer"
            >
              <div className="flex items-center gap-2 text-slate-300">
                <SatIcon size={14} className="text-cyan-400" />
                <span>Satellite Nodes</span>
              </div>
              {globeLayers.showSatellites ? (
                <Eye size={14} className="text-cyan-400" />
              ) : (
                <EyeOff size={14} className="text-slate-600" />
              )}
            </div>

            {/* Labels */}
            <div
              onClick={() => onToggleGlobeLayer('showLabels')}
              className="flex items-center justify-between p-2 rounded-lg hover:bg-slate-800/60 transition cursor-pointer"
            >
              <div className="flex items-center gap-2 text-slate-300">
                <Tag size={14} className="text-cyan-400" />
                <span>Satellite ID Labels</span>
              </div>
              {globeLayers.showLabels ? (
                <Eye size={14} className="text-cyan-400" />
              ) : (
                <EyeOff size={14} className="text-slate-600" />
              )}
            </div>

            {/* Orbits */}
            <div
              onClick={() => onToggleGlobeLayer('showOrbits')}
              className="flex items-center justify-between p-2 rounded-lg hover:bg-slate-800/60 transition cursor-pointer"
            >
              <div className="flex items-center gap-2 text-slate-300">
                <Globe size={14} className="text-yellow-400" />
                <span>Orbital Trajectories</span>
              </div>
              {globeLayers.showOrbits ? (
                <Eye size={14} className="text-yellow-400" />
              ) : (
                <EyeOff size={14} className="text-slate-600" />
              )}
            </div>

            {/* ISL Links */}
            <div
              onClick={() => onToggleGlobeLayer('showIsl')}
              className="flex items-center justify-between p-2 rounded-lg hover:bg-slate-800/60 transition cursor-pointer"
            >
              <div className="flex items-center gap-2 text-slate-300">
                <Share2 size={14} className="text-cyan-400" />
                <span>ISL Laser Crosslinks</span>
              </div>
              {globeLayers.showIsl ? (
                <Eye size={14} className="text-cyan-400" />
              ) : (
                <EyeOff size={14} className="text-slate-600" />
              )}
            </div>

            {/* Feeder Links */}
            <div
              onClick={() => onToggleGlobeLayer('showFeeders')}
              className="flex items-center justify-between p-2 rounded-lg hover:bg-slate-800/60 transition cursor-pointer"
            >
              <div className="flex items-center gap-2 text-slate-300">
                <Radio size={14} className="text-amber-400" />
                <span>Gateway Feeder Beams</span>
              </div>
              {globeLayers.showFeeders ? (
                <Eye size={14} className="text-amber-400" />
              ) : (
                <EyeOff size={14} className="text-slate-600" />
              )}
            </div>

            {/* Access Links */}
            <div
              onClick={() => onToggleGlobeLayer('showAccess')}
              className="flex items-center justify-between p-2 rounded-lg hover:bg-slate-800/60 transition cursor-pointer"
            >
              <div className="flex items-center gap-2 text-slate-300">
                <div className="w-3 h-0.5 bg-blue-400 rounded-full" />
                <span>User Access Beams</span>
              </div>
              {globeLayers.showAccess ? (
                <Eye size={14} className="text-blue-400" />
              ) : (
                <EyeOff size={14} className="text-slate-600" />
              )}
            </div>

            {/* Ground Stations & Users */}
            <div
              onClick={() => onToggleGlobeLayer('showGroundStations')}
              className="flex items-center justify-between p-2 rounded-lg hover:bg-slate-800/60 transition cursor-pointer"
            >
              <div className="flex items-center gap-2 text-slate-300">
                <div className="w-2.5 h-2.5 rounded-full bg-purple-400" />
                <span>Ground Gateways & Terminals</span>
              </div>
              {globeLayers.showGroundStations ? (
                <Eye size={14} className="text-purple-400" />
              ) : (
                <EyeOff size={14} className="text-slate-600" />
              )}
            </div>
          </div>
        </section>

        {/* 6. DASHBOARD & HUD WIDGETS */}
        <section className="space-y-2.5">
          <div className="flex items-center justify-between text-slate-300">
            <span className="font-semibold text-[11px] uppercase tracking-wider flex items-center gap-1.5">
              <LayoutDashboard size={14} className="text-emerald-400" />
              Dashboard Panels
            </span>
          </div>

          <div className="grid grid-cols-2 gap-2">
            <button
              onClick={onToggleKpis}
              className={`p-2.5 rounded-lg border text-left transition cursor-pointer ${
                showKpis
                  ? 'bg-emerald-950/60 border-emerald-700 text-emerald-200'
                  : 'bg-slate-800/50 border-slate-700/60 text-slate-400 hover:text-slate-300'
              }`}
            >
              <div className="flex items-center justify-between">
                <span className="font-semibold text-[11px]">3GPP KPI Cards</span>
                {showKpis ? <CheckSquare size={13} /> : <Square size={13} />}
              </div>
              <div className="text-[10px] text-slate-400">Latency, PDR, Doppler</div>
            </button>

            <button
              onClick={onToggleCharts}
              className={`p-2.5 rounded-lg border text-left transition cursor-pointer ${
                showCharts
                  ? 'bg-cyan-950/60 border-cyan-700 text-cyan-200'
                  : 'bg-slate-800/50 border-slate-700/60 text-slate-400 hover:text-slate-300'
              }`}
            >
              <div className="flex items-center justify-between">
                <span className="font-semibold text-[11px]">Analytics Chart</span>
                {showCharts ? <CheckSquare size={13} /> : <Square size={13} />}
              </div>
              <div className="text-[10px] text-slate-400">Live Recharts Stream</div>
            </button>
          </div>
        </section>
      </div>
    </div>
  );
};
