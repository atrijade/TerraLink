import React from 'react';
import {
  ShieldCheck,
  Timer,
  ArrowUpDown,
  RefreshCw,
  Wifi,
} from 'lucide-react';
import type { SimulationMetrics } from '../../api/types';

interface KpiCardsProps {
  metrics: SimulationMetrics | null;
}

export const KpiCards: React.FC<KpiCardsProps> = ({ metrics }) => {
  const traffic = metrics?.traffic;
  const handovers = metrics?.handovers;
  const radio = metrics?.radio;

  const pdr = traffic?.pdrPercent ?? 99.1;
  const avgLatency = traffic?.avgLatencyMs ?? 24.2;
  const p95Latency = traffic?.p95LatencyMs ?? 34.8;
  const jitter = traffic?.jitterMs ?? 1.2;
  const throughput = traffic?.throughputMbps ?? 850.0;
  const droppedPackets = traffic?.packetsDropped ?? 0;
  const queueDrops = traffic?.dropsQueueOverflow ?? 0;
  const noRouteDrops = traffic?.dropsNoRoute ?? 0;

  const handoverSuccess = handovers?.successRatePercent ?? 98.4;
  const totalHandovers = handovers?.totalAttempts ?? 140;
  const pingPongRate = handovers?.pingPongRatePercent ?? 2.5;

  const meanSinr = radio?.meanSinrDb ?? 19.4;
  const dopplerKhz = (radio?.meanDopplerHz ?? 18500) / 1000.0;

  // Threshold colors for PDR
  const getPdrColor = (val: number) => {
    if (val >= 98.0) return { text: 'text-emerald-400', badge: 'bg-emerald-950/80 text-emerald-300 border-emerald-800/60', label: 'OPTIMAL' };
    if (val >= 92.0) return { text: 'text-amber-400', badge: 'bg-amber-950/80 text-amber-300 border-amber-800/60', label: 'DEGRADED' };
    return { text: 'text-rose-400', badge: 'bg-rose-950/80 text-rose-300 border-rose-800/60', label: 'CRITICAL' };
  };

  const pdrStatus = getPdrColor(pdr);

  return (
    <div className="grid grid-cols-2 md:grid-cols-4 gap-2.5 w-full">
      {/* 1. Packet Delivery Ratio (PDR %) */}
      <div className="bg-slate-900/85 backdrop-blur-md border border-slate-700/60 rounded-xl p-3 shadow-xl flex flex-col justify-between">
        <div className="flex items-center justify-between">
          <div className="flex items-center gap-1.5 text-slate-400 text-xs font-medium">
            <ShieldCheck size={14} className="text-emerald-400" />
            <span>Packet Delivery (PDR)</span>
          </div>
          <span className={`text-[10px] font-bold px-1.5 py-0.5 rounded border ${pdrStatus.badge}`}>
            {pdrStatus.label}
          </span>
        </div>

        <div className="my-1.5 flex items-baseline gap-2">
          <span className={`text-2xl font-black font-mono tracking-tight ${pdrStatus.text}`}>
            {pdr.toFixed(1)}%
          </span>
          <span className="text-[11px] text-slate-400">
            {droppedPackets} dropped
          </span>
        </div>

        <div className="text-[10px] text-slate-400 flex justify-between pt-1 border-t border-slate-800/80">
          <span>Q-Overflow: <strong className="text-slate-300 font-mono">{queueDrops}</strong></span>
          <span>No-Route: <strong className="text-rose-300 font-mono">{noRouteDrops}</strong></span>
        </div>
      </div>

      {/* 2. End-to-End Latency */}
      <div className="bg-slate-900/85 backdrop-blur-md border border-slate-700/60 rounded-xl p-3 shadow-xl flex flex-col justify-between">
        <div className="flex items-center justify-between">
          <div className="flex items-center gap-1.5 text-slate-400 text-xs font-medium">
            <Timer size={14} className="text-cyan-400" />
            <span>E2E Delay / Jitter</span>
          </div>
          <span className="text-[10px] text-slate-400 font-mono">
            ±{jitter.toFixed(1)}ms
          </span>
        </div>

        <div className="my-1.5 flex items-baseline gap-2">
          <span className="text-2xl font-black font-mono tracking-tight text-cyan-300">
            {avgLatency.toFixed(1)}
          </span>
          <span className="text-xs text-slate-400">ms avg</span>
        </div>

        <div className="text-[10px] text-slate-400 flex justify-between pt-1 border-t border-slate-800/80">
          <span>P95 Latency: <strong className="text-amber-300 font-mono">{p95Latency.toFixed(1)} ms</strong></span>
          <span>Target: &lt;50ms</span>
        </div>
      </div>

      {/* 3. Throughput & Network Flow */}
      <div className="bg-slate-900/85 backdrop-blur-md border border-slate-700/60 rounded-xl p-3 shadow-xl flex flex-col justify-between">
        <div className="flex items-center justify-between">
          <div className="flex items-center gap-1.5 text-slate-400 text-xs font-medium">
            <ArrowUpDown size={14} className="text-purple-400" />
            <span>Mesh Throughput</span>
          </div>
          <span className="text-[10px] text-purple-400 bg-purple-950/70 border border-purple-800/50 px-1.5 py-0.5 rounded font-mono">
            3GPP NTN
          </span>
        </div>

        <div className="my-1.5 flex items-baseline gap-2">
          <span className="text-2xl font-black font-mono tracking-tight text-purple-300">
            {throughput.toFixed(1)}
          </span>
          <span className="text-xs text-slate-400">Mbps</span>
        </div>

        <div className="text-[10px] text-slate-400 flex justify-between pt-1 border-t border-slate-800/80">
          <span>Sent: <strong className="text-slate-300 font-mono">{traffic?.packetsSent ?? 120000}</strong></span>
          <span>Delivered: <strong className="text-emerald-300 font-mono">{traffic?.packetsDelivered ?? 119500}</strong></span>
        </div>
      </div>

      {/* 4. Handover & Radio KPIs */}
      <div className="bg-slate-900/85 backdrop-blur-md border border-slate-700/60 rounded-xl p-3 shadow-xl flex flex-col justify-between">
        <div className="flex items-center justify-between">
          <div className="flex items-center gap-1.5 text-slate-400 text-xs font-medium">
            <RefreshCw size={14} className="text-amber-400" />
            <span>Handovers & Radio</span>
          </div>
          <div className="flex items-center gap-1 text-[10px] text-slate-400">
            <Wifi size={11} className="text-cyan-400" />
            <span>{meanSinr.toFixed(1)} dB</span>
          </div>
        </div>

        <div className="my-1.5 flex items-baseline gap-2">
          <span className="text-2xl font-black font-mono tracking-tight text-amber-300">
            {handoverSuccess.toFixed(1)}%
          </span>
          <span className="text-xs text-slate-400">({totalHandovers} total)</span>
        </div>

        <div className="text-[10px] text-slate-400 flex justify-between pt-1 border-t border-slate-800/80">
          <span>Ping-Pong: <strong className="text-slate-300 font-mono">{pingPongRate.toFixed(1)}%</strong></span>
          <span>Doppler: <strong className="text-cyan-300 font-mono">{dopplerKhz.toFixed(1)} kHz</strong></span>
        </div>
      </div>
    </div>
  );
};
