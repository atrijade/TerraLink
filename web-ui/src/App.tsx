import React, { useState } from 'react';
import { useSimulation } from './api/useSimulation';
import { GlobeViewer } from './components/globe/GlobeViewer';
import { TopHeaderHud } from './components/dashboard/TopHeaderHud';
import { TelemetryDashboard } from './components/dashboard/TelemetryDashboard';
import { FaultInjectionPanel } from './components/controls/FaultInjectionPanel';
import { BackendOfflineModal } from './components/common/BackendOfflineModal';

export const App: React.FC = () => {
  const [pollIntervalMs, setPollIntervalMs] = useState<number>(600);
  const [isFaultPanelOpen, setIsFaultPanelOpen] = useState<boolean>(false);

  const {
    satellites,
    topology,
    gateways,
    users,
    metrics,
    status,
    metricHistory,
    selectedEntityId,
    setSelectedEntityId,
    isBackendOnline,
    backendError,
    fetchAll,
    startSimulation,
    stopSimulation,
    injectLinkFailure,
    restoreLink,
  } = useSimulation(pollIntervalMs);

  const activeIsls = topology.filter((l) => l.active).length;
  const hasFailedLinks = topology.some((l) => !l.active || l.status === 'FAILED');

  return (
    <div className="relative w-screen h-screen overflow-hidden bg-slate-950 flex flex-col font-sans select-none">
      {/* 1. Top Header HUD with Branding, Status, Active Node Counts & Controls */}
      <TopHeaderHud
        status={status}
        simTime={metrics?.simTime || 0}
        satellitesCount={satellites.length}
        activeIslCount={activeIsls}
        totalIslCount={topology.length}
        gatewaysCount={gateways.length}
        usersCount={users.length}
        hasFailedLinks={hasFailedLinks}
        isFaultPanelOpen={isFaultPanelOpen}
        onToggleFaultPanel={() => setIsFaultPanelOpen((prev) => !prev)}
        onStartSimulation={startSimulation}
        onStopSimulation={stopSimulation}
      />

      {/* 2. 3D Cesium Earth Viewport */}
      <main className="relative flex-1 w-full h-full">
        <GlobeViewer
          satellites={satellites}
          topology={topology}
          gateways={gateways}
          users={users}
          selectedEntityId={selectedEntityId}
          onSelectEntity={setSelectedEntityId}
        />

        {/* Selected Entity Card Inspector */}
        {selectedEntityId && (
          <div className="absolute top-20 left-4 z-20 bg-slate-900/90 backdrop-blur-md border border-cyan-800/60 rounded-xl p-4 shadow-2xl w-80 text-xs text-slate-200 animate-in fade-in zoom-in-95 duration-100">
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
                        <span className="text-slate-400">ECEF Coordinates:</span>
                        <span className="font-mono text-[10px] text-slate-400">
                          X:{Math.round(sat.ecef?.x || 0)} Y:{Math.round(sat.ecef?.y || 0)}
                        </span>
                      </div>
                      <div className="flex justify-between">
                        <span className="text-slate-400">Orbital Velocity:</span>
                        <span className="font-mono text-emerald-300">7.56 km/s</span>
                      </div>
                      <div className="pt-2 border-t border-slate-800">
                        <button
                          onClick={() => setIsFaultPanelOpen(true)}
                          className="w-full py-1 px-2 bg-slate-800 hover:bg-slate-700 text-cyan-300 rounded text-center font-medium transition cursor-pointer"
                        >
                          Configure ISL Links
                        </button>
                      </div>
                    </>
                  );
                })()}
              </div>
            )}

            {selectedEntityId.startsWith('GW_') && (
              <div className="space-y-1.5">
                {(() => {
                  const gw = gateways.find((g) => g.id === selectedEntityId);
                  if (!gw) return <div>Gateway telemetry unavailable</div>;
                  return (
                    <>
                      <div className="flex justify-between">
                        <span className="text-slate-400">Connected Satellite:</span>
                        <span className="font-mono text-amber-300">{gw.connectedSat}</span>
                      </div>
                      <div className="flex justify-between">
                        <span className="text-slate-400">Throughput:</span>
                        <span className="font-mono text-slate-200">{gw.throughputMbps.toFixed(1)} Mbps</span>
                      </div>
                      <div className="flex justify-between">
                        <span className="text-slate-400">Avg Feeder Delay:</span>
                        <span className="font-mono text-cyan-300">{gw.avgLatencyMs.toFixed(1)} ms</span>
                      </div>
                      <div className="flex justify-between">
                        <span className="text-slate-400">Packets Received:</span>
                        <span className="font-mono text-emerald-300">{gw.packetsReceived.toLocaleString()}</span>
                      </div>
                    </>
                  );
                })()}
              </div>
            )}
          </div>
        )}

        {/* Fault Injection & Operator Control Drawer */}
        <FaultInjectionPanel
          satellites={satellites}
          topology={topology}
          onInjectFailure={injectLinkFailure}
          onRestoreLink={restoreLink}
          isOpen={isFaultPanelOpen}
          onClose={() => setIsFaultPanelOpen(false)}
          pollIntervalMs={pollIntervalMs}
          onSetPollInterval={setPollIntervalMs}
        />
      </main>

      {/* 3. Real-Time Telemetry HUD & Analytics (KPI Cards + Recharts) */}
      <TelemetryDashboard metrics={metrics} history={metricHistory} />

      {/* 4. Backend Offline Warning Modal */}
      {!isBackendOnline && (
        <BackendOfflineModal error={backendError} onRetry={fetchAll} />
      )}
    </div>
  );
};

export default App;
