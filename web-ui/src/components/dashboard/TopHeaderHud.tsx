import React from 'react';
import {
  Satellite as SatIcon,
  Activity,
  Radio,
  Users,
  Play,
  Square,
  Clock,
  Zap,
  ShieldAlert,
  Sliders,
} from 'lucide-react';
import type { ServerStatus } from '../../api/types';

interface TopHeaderHudProps {
  status: ServerStatus;
  isSimRunning: boolean;
  simTime: number;
  satellitesCount: number;
  activeIslCount: number;
  totalIslCount: number;
  gatewaysCount: number;
  usersCount: number;
  hasFailedLinks: boolean;
  isFaultPanelOpen: boolean;
  onToggleFaultPanel: () => void;
  isCustomizationOpen: boolean;
  onToggleCustomization: () => void;
  onStartSimulation: () => void;
  onStopSimulation: () => void;
}

export const TopHeaderHud: React.FC<TopHeaderHudProps> = ({
  status,
  isSimRunning,
  simTime,
  satellitesCount,
  activeIslCount,
  totalIslCount,
  gatewaysCount,
  usersCount,
  hasFailedLinks,
  isFaultPanelOpen,
  onToggleFaultPanel,
  isCustomizationOpen,
  onToggleCustomization,
  onStartSimulation,
  onStopSimulation,
}) => {
  const isRunning = isSimRunning || status.status === 'RUNNING';

  const formatSimTime = (seconds: number) => {
    const mins = Math.floor(seconds / 60);
    const secs = (seconds % 60).toFixed(1);
    return `${mins.toString().padStart(2, '0')}:${secs.padStart(4, '0')}`;
  };

  return (
    <header className="absolute top-4 left-4 right-4 z-20 flex items-center justify-between pointer-events-none">
      {/* Left: Branding & Core Sim Status */}
      <div className="flex items-center gap-3 bg-slate-900/90 backdrop-blur-md border border-slate-700/70 rounded-xl px-4 py-2.5 shadow-2xl pointer-events-auto">
        <div className="flex items-center gap-2.5">
          <div className="w-8 h-8 rounded-lg bg-gradient-to-br from-cyan-500 to-blue-600 flex items-center justify-center shadow-lg shadow-cyan-500/20">
            <Zap size={18} className="text-white" />
          </div>
          <div>
            <div className="flex items-center gap-2">
              <span className="font-extrabold tracking-wider text-slate-100 text-sm">
                TERRALINK
              </span>
              <span className="text-[10px] uppercase font-bold tracking-widest text-cyan-400 bg-cyan-950/80 px-1.5 py-0.5 rounded border border-cyan-800/60">
                5G NTN
              </span>
            </div>
            <div className="text-[10px] text-slate-400 tracking-tight">
              Discrete-Event LEO Mesh Simulation
            </div>
          </div>
        </div>

        <div className="h-6 w-px bg-slate-700 mx-1" />

        {/* Master Simulation Turn ON and Turn OFF Buttons (Always Visible) */}
        <div className="flex items-center gap-1.5 bg-slate-950/80 border border-slate-700/80 rounded-xl p-1 shadow-inner">
          {/* Turn ON Button */}
          <button
            onClick={onStartSimulation}
            disabled={isRunning}
            title={isRunning ? "Simulation is currently ACTIVE" : "Click to Turn ON Simulation"}
            className={`flex items-center gap-1.5 px-3 py-1.5 rounded-lg text-xs font-bold transition-all ${
              isRunning
                ? 'bg-emerald-600 text-white shadow-md shadow-emerald-900/50 cursor-default'
                : 'bg-slate-800/80 hover:bg-emerald-900/70 text-slate-400 hover:text-emerald-200 border border-transparent hover:border-emerald-700/60 cursor-pointer animate-pulse'
            }`}
          >
            <Play size={12} className={isRunning ? 'fill-white text-white' : 'fill-emerald-400 text-emerald-400'} />
            <span>Turn ON</span>
            {isRunning && (
              <span className="w-1.5 h-1.5 rounded-full bg-white animate-ping ml-0.5" />
            )}
          </button>

          {/* Turn OFF Button */}
          <button
            onClick={onStopSimulation}
            disabled={!isRunning}
            title={!isRunning ? "Simulation is currently STOPPED" : "Click to Turn OFF Simulation"}
            className={`flex items-center gap-1.5 px-3 py-1.5 rounded-lg text-xs font-bold transition-all ${
              !isRunning
                ? 'bg-rose-600 text-white shadow-md shadow-rose-900/50 cursor-default'
                : 'bg-slate-800/80 hover:bg-rose-900/70 text-slate-400 hover:text-rose-200 border border-transparent hover:border-rose-700/60 cursor-pointer'
            }`}
          >
            <Square size={12} className={!isRunning ? 'fill-white text-white' : 'fill-rose-400 text-rose-400'} />
            <span>Turn OFF</span>
            {!isRunning && (
              <span className="w-1.5 h-1.5 rounded-full bg-white ml-0.5" />
            )}
          </button>
        </div>

        {/* Sim Time Elapsed */}
        <div className="flex items-center gap-1.5 px-2.5 py-1 rounded-lg bg-slate-800/80 border border-slate-700/50 text-slate-300">
          <Clock size={13} className="text-cyan-400" />
          <span className="text-xs font-mono font-semibold">
            T+{formatSimTime(simTime)}
          </span>
          {!isRunning && (
            <span className="text-[10px] font-bold text-rose-400 uppercase tracking-wider ml-1">
              PAUSED
            </span>
          )}
        </div>
      </div>

      {/* Center/Right: Active Node Counters, Customization, & Fault Injection */}
      <div className="hidden lg:flex items-center gap-3.5 bg-slate-900/90 backdrop-blur-md border border-slate-700/70 rounded-xl px-4 py-2 shadow-2xl pointer-events-auto">
        {/* Satellites */}
        <div className="flex items-center gap-2">
          <div className="p-1.5 rounded-md bg-cyan-950/80 text-cyan-400 border border-cyan-800/50">
            <SatIcon size={14} />
          </div>
          <div>
            <div className="text-[10px] uppercase tracking-wider text-slate-400">
              Satellites
            </div>
            <div className="text-xs font-bold text-cyan-300 font-mono">
              {satellitesCount}
            </div>
          </div>
        </div>

        <div className="h-5 w-px bg-slate-700/80" />

        {/* Active ISL Links */}
        <div className="flex items-center gap-2">
          <div className="p-1.5 rounded-md bg-emerald-950/80 text-emerald-400 border border-emerald-800/50">
            <Activity size={14} />
          </div>
          <div>
            <div className="text-[10px] uppercase tracking-wider text-slate-400">
              Active ISLs
            </div>
            <div className="text-xs font-bold text-emerald-300 font-mono">
              {activeIslCount}{' '}
              <span className="text-[10px] text-slate-400 font-normal">
                / {totalIslCount}
              </span>
            </div>
          </div>
        </div>

        <div className="h-5 w-px bg-slate-700/80" />

        {/* Gateways */}
        <div className="flex items-center gap-2">
          <div className="p-1.5 rounded-md bg-amber-950/80 text-amber-400 border border-amber-800/50">
            <Radio size={14} />
          </div>
          <div>
            <div className="text-[10px] uppercase tracking-wider text-slate-400">
              Gateways
            </div>
            <div className="text-xs font-bold text-amber-300 font-mono">
              {gatewaysCount}
            </div>
          </div>
        </div>

        <div className="h-5 w-px bg-slate-700/80" />

        {/* Users */}
        <div className="flex items-center gap-2">
          <div className="p-1.5 rounded-md bg-purple-950/80 text-purple-400 border border-purple-800/50">
            <Users size={14} />
          </div>
          <div>
            <div className="text-[10px] uppercase tracking-wider text-slate-400">
              Users
            </div>
            <div className="text-xs font-bold text-purple-300 font-mono">
              {usersCount}
            </div>
          </div>
        </div>

        <div className="h-5 w-px bg-slate-700/80" />

        {/* UI Customization Drawer Button */}
        <button
          onClick={onToggleCustomization}
          title="Open UI & Simulation Customization"
          className={`flex items-center gap-1.5 px-3 py-1.5 rounded-lg border text-xs font-semibold transition cursor-pointer ${
            isCustomizationOpen
              ? 'bg-cyan-950 text-cyan-300 border-cyan-700 shadow-md shadow-cyan-950/60'
              : 'bg-slate-800/90 text-slate-300 border-slate-700/60 hover:text-white hover:bg-slate-800'
          }`}
        >
          <Sliders size={14} className={isCustomizationOpen ? 'text-cyan-300' : 'text-slate-400'} />
          <span>Customize UI</span>
        </button>

        {/* Fault Injection Button */}
        <button
          onClick={onToggleFaultPanel}
          title="Open Link Fault Injection Drawer"
          className={`flex items-center gap-1.5 px-3 py-1.5 rounded-lg border text-xs font-semibold transition cursor-pointer ${
            hasFailedLinks
              ? 'bg-rose-950/80 text-rose-300 border-rose-800/80 shadow-lg shadow-rose-950/50'
              : isFaultPanelOpen
              ? 'bg-cyan-950 text-cyan-300 border-cyan-800/80'
              : 'bg-slate-800/90 text-slate-300 border-slate-700/60 hover:text-white hover:bg-slate-800'
          }`}
        >
          <ShieldAlert size={14} className={hasFailedLinks ? 'text-rose-400' : 'text-cyan-400'} />
          <span>Fault Injection</span>
          {hasFailedLinks && (
            <span className="w-2 h-2 rounded-full bg-rose-500 animate-ping" />
          )}
        </button>
      </div>
    </header>
  );
};
