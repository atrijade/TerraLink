import React, { useState } from 'react';
import { WifiOff, RefreshCw, Terminal, AlertTriangle } from 'lucide-react';
import { ntnClient } from '../../api/ntnClient';

interface BackendOfflineModalProps {
  error: string | null;
  onRetry: () => Promise<void>;
}

export const BackendOfflineModal: React.FC<BackendOfflineModalProps> = ({
  error,
  onRetry,
}) => {
  const [isRetrying, setIsRetrying] = useState<boolean>(false);

  const handleRetry = async () => {
    setIsRetrying(true);
    try {
      await onRetry();
    } finally {
      setIsRetrying(false);
    }
  };

  const handleEnableMock = async () => {
    ntnClient.setEnableMockFallback(true);
    await handleRetry();
  };

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center bg-slate-950/80 backdrop-blur-md p-4 animate-in fade-in duration-200">
      <div className="max-w-md w-full bg-slate-900 border border-rose-800/80 rounded-2xl shadow-2xl overflow-hidden flex flex-col text-slate-200">
        {/* Top Warning Banner */}
        <div className="bg-rose-950/90 border-b border-rose-800/80 px-6 py-4 flex items-center gap-3">
          <div className="w-10 h-10 rounded-xl bg-rose-600/30 border border-rose-500/60 flex items-center justify-center shrink-0">
            <WifiOff size={22} className="text-rose-400 animate-pulse" />
          </div>
          <div>
            <h2 className="text-base font-bold text-white tracking-wide">
              Backend Server Offline
            </h2>
            <p className="text-xs text-rose-300">
              C++ NTN-Sim simulation engine is not running
            </p>
          </div>
        </div>

        {/* Content Body */}
        <div className="p-6 space-y-4">
          <div className="bg-slate-950 border border-slate-800 rounded-xl p-3 text-xs font-mono text-slate-400 break-words flex items-start gap-2">
            <AlertTriangle size={15} className="text-amber-400 shrink-0 mt-0.5" />
            <span>{error || 'Unable to establish HTTP connection to http://localhost:8080'}</span>
          </div>

          <div className="space-y-2">
            <div className="flex items-center gap-1.5 text-xs font-semibold text-slate-300 uppercase tracking-wider">
              <Terminal size={14} className="text-cyan-400" />
              <span>How to start the C++ Backend:</span>
            </div>
            <div className="bg-slate-950/90 border border-slate-800 rounded-xl p-3 text-xs font-mono text-slate-300 space-y-1">
              <div className="text-slate-500"># In PowerShell:</div>
              <div className="text-cyan-300">cd c:\TerraLink\ntn-sim</div>
              <div className="text-emerald-400">..\build\ntn_sim_bin.exe</div>
            </div>
          </div>

          {/* Action Buttons */}
          <div className="flex flex-col gap-2 pt-2">
            <button
              onClick={handleRetry}
              disabled={isRetrying}
              className="w-full flex items-center justify-center gap-2 py-2.5 px-4 bg-cyan-600 hover:bg-cyan-500 disabled:bg-slate-800 text-white font-bold text-xs rounded-xl shadow-lg shadow-cyan-900/30 transition cursor-pointer"
            >
              <RefreshCw size={14} className={isRetrying ? 'animate-spin' : ''} />
              <span>{isRetrying ? 'Checking Port 8080...' : 'Retry Connection'}</span>
            </button>

            <button
              onClick={handleEnableMock}
              className="w-full py-2 px-4 bg-slate-800 hover:bg-slate-700 text-slate-400 hover:text-slate-200 text-xs rounded-xl transition border border-slate-700/60"
            >
              Or switch back to Simulated Mock Engine
            </button>
          </div>
        </div>
      </div>
    </div>
  );
};
