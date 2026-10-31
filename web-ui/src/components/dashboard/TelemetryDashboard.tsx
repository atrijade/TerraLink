import React, { useState } from 'react';
import { ChevronDown, ChevronUp, BarChart3 } from 'lucide-react';
import { KpiCards } from './KpiCards';
import { RealtimeChart } from './RealtimeChart';
import type { SimulationMetrics } from '../../api/types';
import type { MetricHistoryPoint } from '../../api/useSimulation';

interface TelemetryDashboardProps {
  metrics: SimulationMetrics | null;
  history: MetricHistoryPoint[];
}

export const TelemetryDashboard: React.FC<TelemetryDashboardProps> = ({
  metrics,
  history,
}) => {
  const [isExpanded, setIsExpanded] = useState<boolean>(true);

  return (
    <aside className="absolute bottom-4 left-4 right-4 z-20 flex flex-col pointer-events-none">
      <div className="flex flex-col gap-2.5 max-w-7xl mx-auto w-full pointer-events-auto">
        {/* Toggle Bar */}
        <div className="flex items-center justify-between">
          <button
            onClick={() => setIsExpanded((prev) => !prev)}
            className="flex items-center gap-2 px-3 py-1 bg-slate-900/90 hover:bg-slate-800 backdrop-blur-md border border-slate-700/60 rounded-lg text-xs font-semibold text-slate-300 hover:text-cyan-400 shadow-xl transition cursor-pointer"
          >
            <BarChart3 size={13} className="text-cyan-400" />
            <span>3GPP NTN Performance & Analytics</span>
            {isExpanded ? <ChevronDown size={14} /> : <ChevronUp size={14} />}
          </button>
        </div>

        {/* Expandable KPI & Chart Cards */}
        {isExpanded && (
          <div className="flex flex-col lg:flex-row gap-2.5 w-full items-stretch">
            {/* KPI Summary Cards */}
            <div className="flex-1">
              <KpiCards metrics={metrics} />
            </div>

            {/* Recharts Live Stream Chart */}
            <div className="w-full lg:w-[460px]">
              <RealtimeChart history={history} />
            </div>
          </div>
        )}
      </div>
    </aside>
  );
};
