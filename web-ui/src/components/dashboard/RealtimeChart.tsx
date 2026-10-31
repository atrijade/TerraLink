import React, { useState } from 'react';
import {
  ResponsiveContainer,
  LineChart,
  Line,
  XAxis,
  YAxis,
  Tooltip,
  CartesianGrid,
  Legend,
} from 'recharts';
import type { MetricHistoryPoint } from '../../api/useSimulation';
import { Activity } from 'lucide-react';

interface RealtimeChartProps {
  history: MetricHistoryPoint[];
}

export const RealtimeChart: React.FC<RealtimeChartProps> = ({ history }) => {
  const [activeTab, setActiveTab] = useState<'latency' | 'throughput'>('latency');

  return (
    <div className="bg-slate-900/85 backdrop-blur-md border border-slate-700/60 rounded-xl p-3.5 shadow-xl flex flex-col w-full">
      {/* Header & Mode Switcher */}
      <div className="flex items-center justify-between pb-2 mb-2 border-b border-slate-800/80">
        <div className="flex items-center gap-2">
          <Activity size={15} className="text-cyan-400" />
          <span className="text-xs font-bold text-slate-200 tracking-wide">
            Real-Time Telemetry Stream
          </span>
          <span className="text-[10px] text-slate-500 font-mono">
            ({history.length} pts)
          </span>
        </div>

        <div className="flex items-center gap-1 bg-slate-800/90 p-0.5 rounded-lg border border-slate-700/60 text-[11px]">
          <button
            onClick={() => setActiveTab('latency')}
            className={`px-2 py-0.5 rounded transition font-medium ${
              activeTab === 'latency'
                ? 'bg-cyan-950 text-cyan-300 border border-cyan-800/60'
                : 'text-slate-400 hover:text-slate-200'
            }`}
          >
            Latency (ms)
          </button>
          <button
            onClick={() => setActiveTab('throughput')}
            className={`px-2 py-0.5 rounded transition font-medium ${
              activeTab === 'throughput'
                ? 'bg-purple-950 text-purple-300 border border-purple-800/60'
                : 'text-slate-400 hover:text-slate-200'
            }`}
          >
            Throughput (Mbps)
          </button>
        </div>
      </div>

      {/* Recharts Line Graph */}
      <div className="w-full h-44">
        {history.length === 0 ? (
          <div className="w-full h-full flex items-center justify-center text-xs text-slate-500">
            Awaiting telemetry ticks...
          </div>
        ) : (
          <ResponsiveContainer width="100%" height="100%">
            <LineChart
              data={history}
              margin={{ top: 5, right: 10, left: -20, bottom: 0 }}
            >
              <CartesianGrid strokeDasharray="3 3" stroke="#1e293b" />
              <XAxis
                dataKey="time"
                stroke="#64748b"
                tick={{ fontSize: 9, fill: '#64748b' }}
                interval="preserveStartEnd"
              />
              <YAxis
                stroke="#64748b"
                tick={{ fontSize: 9, fill: '#64748b' }}
                domain={activeTab === 'latency' ? [0, 'dataMax + 15'] : [700, 'dataMax + 100']}
              />
              <Tooltip
                contentStyle={{
                  backgroundColor: 'rgba(15, 23, 42, 0.95)',
                  borderColor: '#334155',
                  borderRadius: '0.5rem',
                  fontSize: '11px',
                  boxShadow: '0 10px 25px -5px rgba(0, 0, 0, 0.5)',
                }}
                labelStyle={{ color: '#94a3b8', marginBottom: '2px' }}
              />
              <Legend
                wrapperStyle={{ fontSize: '10px', paddingTop: '4px' }}
                iconType="circle"
              />

              {activeTab === 'latency' ? (
                <>
                  <Line
                    type="monotone"
                    dataKey="avgLatencyMs"
                    name="Mean Latency (ms)"
                    stroke="#38bdf8"
                    strokeWidth={2}
                    dot={false}
                    isAnimationActive={false}
                  />
                  <Line
                    type="monotone"
                    dataKey="p95LatencyMs"
                    name="P95 Latency (ms)"
                    stroke="#f59e0b"
                    strokeWidth={1.5}
                    strokeDasharray="4 4"
                    dot={false}
                    isAnimationActive={false}
                  />
                </>
              ) : (
                <>
                  <Line
                    type="monotone"
                    dataKey="throughputMbps"
                    name="Throughput (Mbps)"
                    stroke="#c084fc"
                    strokeWidth={2}
                    dot={false}
                    isAnimationActive={false}
                  />
                  <Line
                    type="monotone"
                    dataKey="pdrPercent"
                    name="PDR (%)"
                    stroke="#10b981"
                    strokeWidth={1.5}
                    dot={false}
                    isAnimationActive={false}
                  />
                </>
              )}
            </LineChart>
          </ResponsiveContainer>
        )}
      </div>
    </div>
  );
};
