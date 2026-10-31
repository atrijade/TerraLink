import React from 'react';
import { useSimulation } from './api/useSimulation';
import { GlobeViewer } from './components/globe/GlobeViewer';

export const App: React.FC = () => {
  const {
    satellites,
    topology,
    gateways,
    users,
    selectedEntityId,
    setSelectedEntityId,
    status,
  } = useSimulation(600);

  return (
    <div className="relative w-screen h-screen overflow-hidden bg-slate-950 flex flex-col font-sans select-none">
      {/* 3D Earth Globe Viewport */}
      <main className="relative flex-1 w-full h-full">
        <GlobeViewer
          satellites={satellites}
          topology={topology}
          gateways={gateways}
          users={users}
          selectedEntityId={selectedEntityId}
          onSelectEntity={setSelectedEntityId}
        />

        {/* Floating Quick Status HUD */}
        <div className="absolute top-4 left-4 z-20 flex items-center gap-3 bg-slate-900/85 backdrop-blur-md border border-slate-700/60 rounded-xl px-4 py-2.5 shadow-2xl text-xs">
          <div className="flex items-center gap-2">
            <span
              className={`w-2.5 h-2.5 rounded-full ${
                status.status === 'RUNNING'
                  ? 'bg-emerald-400 animate-pulse'
                  : 'bg-rose-500'
              }`}
            />
            <span className="font-bold tracking-wider text-slate-100 text-sm">
              TERRALINK NTN-SIM
            </span>
          </div>

          <div className="h-4 w-px bg-slate-700 mx-1" />

          <div className="flex items-center gap-4 text-slate-300">
            <div>
              <span className="text-slate-400 text-[10px] block uppercase tracking-wider">
                Sats
              </span>
              <span className="font-semibold text-cyan-400 text-xs">
                {satellites.length}
              </span>
            </div>
            <div>
              <span className="text-slate-400 text-[10px] block uppercase tracking-wider">
                Active ISLs
              </span>
              <span className="font-semibold text-emerald-400 text-xs">
                {topology.filter((l) => l.active).length}
              </span>
            </div>
            <div>
              <span className="text-slate-400 text-[10px] block uppercase tracking-wider">
                Gateways
              </span>
              <span className="font-semibold text-amber-400 text-xs">
                {gateways.length}
              </span>
            </div>
            <div>
              <span className="text-slate-400 text-[10px] block uppercase tracking-wider">
                Users
              </span>
              <span className="font-semibold text-purple-400 text-xs">
                {users.length}
              </span>
            </div>
          </div>

          {status.isMockMode && (
            <span className="ml-2 px-2 py-0.5 rounded bg-cyan-950/80 text-cyan-300 border border-cyan-800/60 text-[10px] font-medium tracking-wide">
              SIM ENGINE (MOCK)
            </span>
          )}
        </div>

        {/* Selected Entity Card */}
        {selectedEntityId && (
          <div className="absolute bottom-6 left-6 z-20 bg-slate-900/90 backdrop-blur-md border border-cyan-800/60 rounded-xl p-4 shadow-2xl w-80 text-xs text-slate-200">
            <div className="flex items-center justify-between pb-2 mb-2 border-b border-slate-700/60">
              <span className="font-bold text-cyan-400 text-sm">
                {selectedEntityId}
              </span>
              <button
                onClick={() => setSelectedEntityId(null)}
                className="text-slate-400 hover:text-white px-1.5 py-0.5 rounded hover:bg-slate-800"
              >
                ✕
              </button>
            </div>

            {selectedEntityId.startsWith('SAT_') && (
              <div className="space-y-1.5">
                {(() => {
                  const sat = satellites.find((s) => s.id === selectedEntityId);
                  if (!sat) return <div>Satellite telemetry unavailable</div>;
                  return (
                    <>
                      <div className="flex justify-between">
                        <span className="text-slate-400">Altitude:</span>
                        <span className="font-mono text-cyan-300">
                          {Math.round(sat.altKm || sat.altitudeKm || 600)} km
                        </span>
                      </div>
                      <div className="flex justify-between">
                        <span className="text-slate-400">Latitude / Longitude:</span>
                        <span className="font-mono text-slate-300">
                          {sat.lat.toFixed(2)}°, {sat.lon.toFixed(2)}°
                        </span>
                      </div>
                      <div className="flex justify-between">
                        <span className="text-slate-400">Velocity:</span>
                        <span className="font-mono text-slate-300">7.56 km/s</span>
                      </div>
                    </>
                  );
                })()}
              </div>
            )}
          </div>
        )}
      </main>
    </div>
  );
};

export default App;
