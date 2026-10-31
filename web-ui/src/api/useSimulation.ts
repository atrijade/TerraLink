import { useState, useEffect, useCallback, useRef } from 'react';
import { ntnClient } from './ntnClient';
import type {
  Satellite,
  TopologyLink,
  Gateway,
  User,
  SimulationMetrics,
  ServerStatus,
} from './types';

export interface MetricHistoryPoint {
  time: string;
  simTime: number;
  avgLatencyMs: number;
  p95LatencyMs: number;
  pdrPercent: number;
  throughputMbps: number;
  droppedPackets: number;
}

export function useSimulation(pollIntervalMs: number = 750) {
  const [satellites, setSatellites] = useState<Satellite[]>([]);
  const [topology, setTopology] = useState<TopologyLink[]>([]);
  const [gateways, setGateways] = useState<Gateway[]>([]);
  const [users, setUsers] = useState<User[]>([]);
  const [metrics, setMetrics] = useState<SimulationMetrics | null>(null);
  const [status, setStatus] = useState<ServerStatus>({
    status: 'RUNNING',
    port: 8080,
    controllerConnected: true,
    isMockMode: true,
  });
  const [metricHistory, setMetricHistory] = useState<MetricHistoryPoint[]>([]);
  const [selectedEntityId, setSelectedEntityId] = useState<string | null>(null);
  const [isLoading, setIsLoading] = useState<boolean>(true);

  const isPollingRef = useRef<boolean>(false);

  const fetchAll = useCallback(async () => {
    if (isPollingRef.current) return;
    isPollingRef.current = true;

    try {
      const [satsData, topoData, gwData, userData, metricsData, statusData] =
        await Promise.all([
          ntnClient.fetchSatellites(),
          ntnClient.fetchTopology(),
          ntnClient.fetchGateways(),
          ntnClient.fetchUsers(),
          ntnClient.fetchMetrics(),
          ntnClient.fetchStatus(),
        ]);

      setSatellites(satsData);
      setTopology(topoData.links || []);
      setGateways(gwData);
      setUsers(userData);
      setMetrics(metricsData);
      setStatus(statusData);

      if (metricsData && metricsData.traffic) {
        const point: MetricHistoryPoint = {
          time: new Date().toLocaleTimeString(),
          simTime: Math.round((metricsData.simTime || 0) * 10) / 10,
          avgLatencyMs: Math.round((metricsData.traffic.avgLatencyMs || 0) * 10) / 10,
          p95LatencyMs: Math.round((metricsData.traffic.p95LatencyMs || 0) * 10) / 10,
          pdrPercent: Math.round((metricsData.traffic.pdrPercent || 0) * 10) / 10,
          throughputMbps: Math.round((metricsData.traffic.throughputMbps || 0) * 10) / 10,
          droppedPackets: metricsData.traffic.packetsDropped || 0,
        };

        setMetricHistory((prev) => {
          const next = [...prev, point];
          return next.length > 25 ? next.slice(next.length - 25) : next;
        });
      }
    } catch (err) {
      console.warn('Error fetching simulation telemetry:', err);
    } finally {
      setIsLoading(false);
      isPollingRef.current = false;
    }
  }, []);

  useEffect(() => {
    fetchAll();
    const intervalId = setInterval(fetchAll, pollIntervalMs);
    return () => clearInterval(intervalId);
  }, [fetchAll, pollIntervalMs]);

  const handleStart = async () => {
    await ntnClient.startSimulation();
    await fetchAll();
  };

  const handleStop = async () => {
    await ntnClient.stopSimulation();
    await fetchAll();
  };

  const handleInjectFailure = async (source: string, target: string, bidirectional = true) => {
    const res = await ntnClient.injectLinkFailure(source, target, bidirectional);
    await fetchAll();
    return res.success;
  };

  const handleRestoreLink = async (source: string, target: string, bidirectional = true) => {
    const res = await ntnClient.restoreLink(source, target, bidirectional);
    await fetchAll();
    return res.success;
  };

  return {
    satellites,
    topology,
    gateways,
    users,
    metrics,
    status,
    metricHistory,
    selectedEntityId,
    setSelectedEntityId,
    isLoading,
    fetchAll,
    startSimulation: handleStart,
    stopSimulation: handleStop,
    injectLinkFailure: handleInjectFailure,
    restoreLink: handleRestoreLink,
  };
}
