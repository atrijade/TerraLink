import React, { useState } from 'react';
import {
  AlertTriangle,
  RotateCcw,
  ShieldAlert,
  CheckCircle2,
  Dice5,
  Sliders,
  X,
  Radio,
} from 'lucide-react';
import type { Satellite, TopologyLink } from '../../api/types';

interface FaultInjectionPanelProps {
  satellites: Satellite[];
  topology: TopologyLink[];
  onInjectFailure: (source: string, target: string, bidirectional?: boolean) => Promise<boolean>;
  onRestoreLink: (source: string, target: string, bidirectional?: boolean) => Promise<boolean>;
  isOpen: boolean;
  onClose: () => void;
  pollIntervalMs: number;
  onSetPollInterval: (ms: number) => void;
}

export const FaultInjectionPanel: React.FC<FaultInjectionPanelProps> = ({
  satellites,
  topology,
  onInjectFailure,
  onRestoreLink,
  isOpen,
  onClose,
  pollIntervalMs,
  onSetPollInterval,
}) => {
  const [selectedSource, setSelectedSource] = useState<string>('');
  const [selectedTarget, setSelectedTarget] = useState<string>('');
  const [isSubmitting, setIsSubmitting] = useState<boolean>(false);
  const [statusMessage, setStatusMessage] = useState<{
    type: 'success' | 'error';
    text: string;
  } | null>(null);

  // Active and Failed links
  const activeLinks = topology.filter((l) => l.active);
  const failedLinks = topology.filter((l) => !l.active || l.status === 'FAILED');

  // Set default selection when links are available
  React.useEffect(() => {
    if (activeLinks.length > 0 && !selectedSource && !selectedTarget) {
      setSelectedSource(activeLinks[0].src);
      setSelectedTarget(activeLinks[0].dst);
    }
  }, [activeLinks, selectedSource, selectedTarget]);

  const handleSeverLink = async () => {
    if (!selectedSource || !selectedTarget || selectedSource === selectedTarget) {
      setStatusMessage({ type: 'error', text: 'Select two distinct satellite nodes' });
      return;
    }

    setIsSubmitting(true);
    setStatusMessage(null);
    try {
      const ok = await onInjectFailure(selectedSource, selectedTarget, true);
      if (ok) {
        setStatusMessage({
          type: 'success',
          text: `Severed link between ${selectedSource} and ${selectedTarget}`,
        });
      } else {
        setStatusMessage({ type: 'error', text: 'Failed to inject link outage' });
      }
    } catch {
      setStatusMessage({ type: 'error', text: 'Network request error' });
    } finally {
      setIsSubmitting(false);
    }
  };

  const handleRestore = async (src: string, dst: string) => {
    setIsSubmitting(true);
    setStatusMessage(null);
    try {
      const ok = await onRestoreLink(src, dst, true);
      if (ok) {
        setStatusMessage({
          type: 'success',
          text: `Restored link between ${src} and ${dst}`,
        });
      } else {
        setStatusMessage({ type: 'error', text: 'Failed to restore link' });
      }
    } catch {
      setStatusMessage({ type: 'error', text: 'Network request error' });
    } finally {
      setIsSubmitting(false);
    }
  };

  const handleRestoreAll = async () => {
    setIsSubmitting(true);
    setStatusMessage(null);
    try {
      for (const link of failedLinks) {
        await onRestoreLink(link.src, link.dst, true);
      }
      setStatusMessage({ type: 'success', text: 'All severed links restored to operational status' });
    } finally {
      setIsSubmitting(false);
    }
  };

  const handleRandomOutage = async () => {
    if (activeLinks.length === 0) return;
    const randomLink = activeLinks[Math.floor(Math.random() * activeLinks.length)];
    setSelectedSource(randomLink.src);
    setSelectedTarget(randomLink.dst);
    await onInjectFailure(randomLink.src, randomLink.dst, true);
    setStatusMessage({
      type: 'success',
      text: `Random fault injected: ${randomLink.src} <-> ${randomLink.dst}`,
    });
  };

  if (!isOpen) return null;

  return (
    <div className="absolute top-20 right-4 z-30 w-96 bg-slate-900/95 backdrop-blur-xl border border-slate-700/70 rounded-2xl shadow-2xl overflow-hidden flex flex-col text-slate-200 animate-in fade-in zoom-in-95 duration-150">
      {/* Panel Header */}
      <div className="flex items-center justify-between px-4 py-3 border-b border-slate-800 bg-slate-950/60">
        <div className="flex items-center gap-2">
          <div className="p-1 rounded bg-rose-950/80 text-rose-400 border border-rose-800/60">
            <ShieldAlert size={16} />
          </div>
          <div>
            <span className="font-bold text-sm tracking-wide text-slate-100">
              Interactive Fault Injection
            </span>
            <div className="text-[10px] text-slate-400">
              Dynamic ISL Link Failure & Recovery
            </div>
          </div>
        </div>
        <button
          onClick={onClose}
          className="text-slate-400 hover:text-white p-1 rounded-lg hover:bg-slate-800 transition"
        >
          <X size={16} />
        </button>
      </div>

      <div className="p-4 space-y-4 max-h-[80vh] overflow-y-auto">
        {/* Status Toast Banner */}
        {statusMessage && (
          <div
            className={`flex items-center gap-2 px-3 py-2 rounded-lg text-xs font-medium border ${
              statusMessage.type === 'success'
                ? 'bg-emerald-950/80 text-emerald-300 border-emerald-800/60'
                : 'bg-rose-950/80 text-rose-300 border-rose-800/60'
            }`}
          >
            {statusMessage.type === 'success' ? (
              <CheckCircle2 size={14} className="text-emerald-400 shrink-0" />
            ) : (
              <AlertTriangle size={14} className="text-rose-400 shrink-0" />
            )}
            <span className="leading-tight">{statusMessage.text}</span>
          </div>
        )}

        {/* Section 1: Choose Link to Sever */}
        <div className="space-y-2">
          <label className="text-[11px] font-semibold text-slate-400 tracking-wider uppercase block">
            Target ISL Link
          </label>

          {/* Quick Select from Active ISLs */}
          <select
            value={`${selectedSource}---${selectedTarget}`}
            onChange={(e) => {
              const [src, dst] = e.target.value.split('---');
              if (src && dst) {
                setSelectedSource(src);
                setSelectedTarget(dst);
              }
            }}
            className="w-full bg-slate-950 border border-slate-700 rounded-lg px-2.5 py-1.5 text-xs text-slate-200 focus:outline-none focus:border-cyan-500 font-mono"
          >
            <option value="">-- Choose from {activeLinks.length} Active ISLs --</option>
            {activeLinks.map((link) => (
              <option
                key={`${link.src}---${link.dst}`}
                value={`${link.src}---${link.dst}`}
              >
                {link.src} ↔ {link.dst} ({link.distanceKm.toFixed(0)} km, {link.delayMs.toFixed(1)} ms)
              </option>
            ))}
          </select>

          {/* Manual Satellite Pair Selectors */}
          <div className="grid grid-cols-2 gap-2 pt-1">
            <div>
              <span className="text-[10px] text-slate-400 block mb-1">Source Node</span>
              <select
                value={selectedSource}
                onChange={(e) => setSelectedSource(e.target.value)}
                className="w-full bg-slate-950 border border-slate-700 rounded-lg px-2 py-1.5 text-xs text-slate-200 focus:outline-none focus:border-cyan-500 font-mono"
              >
                {satellites.map((s) => (
                  <option key={s.id} value={s.id}>
                    {s.id}
                  </option>
                ))}
              </select>
            </div>

            <div>
              <span className="text-[10px] text-slate-400 block mb-1">Target Node</span>
              <select
                value={selectedTarget}
                onChange={(e) => setSelectedTarget(e.target.value)}
                className="w-full bg-slate-950 border border-slate-700 rounded-lg px-2 py-1.5 text-xs text-slate-200 focus:outline-none focus:border-cyan-500 font-mono"
              >
                {satellites.map((s) => (
                  <option key={s.id} value={s.id}>
                    {s.id}
                  </option>
                ))}
              </select>
            </div>
          </div>

          {/* Action Trigger Buttons */}
          <div className="flex gap-2 pt-2">
            <button
              onClick={handleSeverLink}
              disabled={isSubmitting || !selectedSource || !selectedTarget}
              className="flex-1 flex items-center justify-center gap-1.5 px-3 py-2 bg-rose-600 hover:bg-rose-500 disabled:bg-slate-800 disabled:text-slate-600 text-white rounded-lg text-xs font-bold transition shadow-lg shadow-rose-900/30 cursor-pointer"
            >
              <AlertTriangle size={14} />
              <span>Sever Link</span>
            </button>

            <button
              onClick={() => handleRestore(selectedSource, selectedTarget)}
              disabled={isSubmitting || !selectedSource || !selectedTarget}
              className="flex-1 flex items-center justify-center gap-1.5 px-3 py-2 bg-emerald-600 hover:bg-emerald-500 disabled:bg-slate-800 disabled:text-slate-600 text-white rounded-lg text-xs font-bold transition shadow-lg shadow-emerald-900/30 cursor-pointer"
            >
              <RotateCcw size={14} />
              <span>Restore Link</span>
            </button>
          </div>

          <button
            onClick={handleRandomOutage}
            disabled={isSubmitting || activeLinks.length === 0}
            className="w-full flex items-center justify-center gap-1.5 px-3 py-1.5 bg-slate-800 hover:bg-slate-700 border border-slate-700 text-slate-300 rounded-lg text-xs font-medium transition cursor-pointer"
          >
            <Dice5 size={14} className="text-amber-400" />
            <span>Simulate Random Outage</span>
          </button>
        </div>

        {/* Section 2: Severed Links List */}
        <div className="space-y-2 pt-2 border-t border-slate-800">
          <div className="flex items-center justify-between">
            <div className="flex items-center gap-1.5">
              <span className="text-[11px] font-semibold text-slate-400 tracking-wider uppercase">
                Severed Links
              </span>
              <span className="px-1.5 py-0.2 rounded-full text-[10px] font-mono bg-rose-950 text-rose-300 border border-rose-800/60">
                {failedLinks.length}
              </span>
            </div>

            {failedLinks.length > 0 && (
              <button
                onClick={handleRestoreAll}
                disabled={isSubmitting}
                className="text-[11px] text-cyan-400 hover:text-cyan-300 underline font-medium cursor-pointer"
              >
                Restore All
              </button>
            )}
          </div>

          {failedLinks.length === 0 ? (
            <div className="text-center py-3 px-2 rounded-lg bg-slate-950/50 border border-slate-800/80 text-xs text-slate-500 flex items-center justify-center gap-1.5">
              <Radio size={13} className="text-emerald-500" />
              <span>All constellation ISL links fully operational</span>
            </div>
          ) : (
            <div className="space-y-1.5 max-h-36 overflow-y-auto pr-1">
              {failedLinks.map((link) => (
                <div
                  key={`${link.src}---${link.dst}`}
                  className="flex items-center justify-between px-2.5 py-1.5 rounded-lg bg-rose-950/40 border border-rose-900/60 text-xs"
                >
                  <div className="flex items-center gap-1.5">
                    <span className="w-2 h-2 rounded-full bg-rose-500 animate-pulse" />
                    <span className="font-mono text-slate-200">
                      {link.src} ↔ {link.dst}
                    </span>
                  </div>

                  <button
                    onClick={() => handleRestore(link.src, link.dst)}
                    disabled={isSubmitting}
                    className="px-2 py-0.5 bg-slate-800 hover:bg-slate-700 text-emerald-400 rounded text-[11px] font-medium border border-slate-700 transition"
                  >
                    Restore
                  </button>
                </div>
              ))}
            </div>
          )}
        </div>

        {/* Section 3: Telemetry Stream Rate */}
        <div className="space-y-2 pt-2 border-t border-slate-800">
          <div className="flex items-center justify-between text-xs text-slate-400">
            <div className="flex items-center gap-1.5">
              <Sliders size={13} className="text-cyan-400" />
              <span className="text-[11px] font-semibold tracking-wider uppercase">
                Telemetry Rate
              </span>
            </div>
            <span className="font-mono text-slate-300">{pollIntervalMs} ms</span>
          </div>

          <div className="grid grid-cols-4 gap-1">
            {[250, 500, 1000, 2000].map((rate) => (
              <button
                key={rate}
                onClick={() => onSetPollInterval(rate)}
                className={`py-1 rounded text-[11px] font-mono font-medium transition ${
                  pollIntervalMs === rate
                    ? 'bg-cyan-950 text-cyan-300 border border-cyan-800/80 font-bold'
                    : 'bg-slate-950 text-slate-400 hover:bg-slate-800'
                }`}
              >
                {rate >= 1000 ? `${rate / 1000}s` : `${rate}ms`}
              </button>
            ))}
          </div>
        </div>
      </div>
    </div>
  );
};
