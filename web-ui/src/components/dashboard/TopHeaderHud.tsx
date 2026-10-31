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
} from 'lucide-react';
import type { ServerStatus } from '../../api/types';

interface TopHeaderHudProps {
  status: ServerStatus;
  simTime: number;
  satellitesCount: number;
  activeIslCount: number;
  totalIslCount: number;
  gatewaysCount: number;
  usersCount: number;
  hasFailedLinks: boolean;
  isFaultPanelOpen: boolean;
  onToggleFaultPanel: () => void;
  onStartSimulation: () => void;
  onStopSimulation: () => void;
}

export const TopHeaderHud: React.FC<TopHeaderHudProps> = ({
  status,
  simTime,
  satellitesCount,
  activeIslCount,
  totalIslCount,
  gatewaysCount,
  usersCount,
  hasFailedLinks,
  isFaultPanelOpen,
  onToggleFaultPanel,
  onStartSimulation,
  onStopSimulation,
}) => {
  const isRunning = status.status === 'RUNNING';

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

        {/* Running / Stopped Badge */}
        <div className="flex items-center gap-2 px-2.5 py-1 rounded-lg bg-slate-800/80 border border-slate-700/50">
          <span
            className={`w-2 h-2 rounded-full ${
              isRunning ? 'bg-emerald-400 animate-pulse' : 'bg-rose-500'
            }`}
          />
          <span
            className={`text-xs font-semibold tracking-wide ${
              isRunning ? 'text-emerald-300' : 'text-rose-400'
            }`}
          >
            {status.status}
          </span>
        </div>

        {/* Sim Time Elapsed */}
        <div className="flex items-center gap-1.5 px-2.5 py-1 rounded-lg bg-slate-800/80 border border-slate-700/50 text-slate-300">
          <Clock size={13} className="text-cyan-400" />
          <span className="text-xs font-mono font-semibold">
            T+{formatSimTime(simTime)}
          </span>
        </div>

        {/* Play/Stop Controls */}
        <div className="flex items-center gap-1 ml-1">
          {isRunning ? (
            <button
              onClick={onStopSimulation}
              title="Stop Simulation"
              className="flex items-center gap-1.5 px-2.5 py-1 bg-rose-950/70 hover:bg-rose-900 border border-rose-800/60 text-rose-200 rounded-lg text-xs font-medium transition cursor-pointer"
            >
              <Square size={12} className="fill-rose-400 text-rose-400" />
              <span>Stop</span>
            </button>
          ) : (
            <button
              onClick={onStartSimulation}
              title="Start Simulation"
              className="flex items-center gap-1.5 px-2.5 py-1 bg-emerald-950/70 hover:bg-emerald-900 border border-emerald-800/60 text-emerald-200 rounded-lg text-xs font-medium transition cursor-pointer"
            >
              <Play size={12} className="fill-emerald-400 text-emerald-400" />
              <span>Start</span>
            </button>
          )}
        </div>
      </div>

      {/* Center/Right: Active Node Counters & Fault Injection Trigger */}
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

        {/* Fault Injection Button */}
        <button
          onClick={onToggleFaultPanel}
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
