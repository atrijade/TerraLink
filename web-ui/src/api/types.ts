/**
 * NTN-Sim REST API & Data Model Type Definitions
 */

export interface EcefCoordinates {
  x: number;
  y: number;
  z: number;
}

export interface Satellite {
  id: string;
  lat: number;
  lon: number;
  altKm: number;
  altitudeKm?: number;
  ecef: EcefCoordinates;
  velKms: EcefCoordinates;
  orbitPlane?: number;
}

export interface TopologyLink {
  src: string;
  dst: string;
  distanceKm: number;
  delayMs: number;
  dataRateGbps: number;
  active: boolean;
  isInterPlane: boolean;
  type?: 'ISL' | 'FEEDER';
  status?: 'ACTIVE' | 'FAILED';
}

export interface TopologyResponse {
  links: TopologyLink[];
}

export interface GatewayFeederProfile {
  carrierFreqGhz: number;
  bandwidthMhz: number;
  dataRateGbps: number;
  minElevationDeg: number;
}

export interface Gateway {
  id: string;
  lat: number;
  lon: number;
  altM: number;
  connectedSat: string;
  hasActiveFeeder: boolean;
  throughputMbps: number;
  jitterMs: number;
  avgLatencyMs: number;
  packetsReceived: number;
  packetsLost: number;
  packetsOutOfOrder: number;
  utilizationPercent: number;
  feederProfile?: GatewayFeederProfile;
}

export interface User {
  id: string;
  lat: number;
  lon: number;
  altM: number;
  servingSat: string;
  trafficRatePps: number;
  sinrDb?: number;
  rsrpDbm?: number;
}

export interface TrafficKPIs {
  packetsSent: number;
  packetsDelivered: number;
  packetsDropped: number;
  pdrPercent: number;
  avgLatencyMs: number;
  medianLatencyMs: number;
  p95LatencyMs: number;
  p99LatencyMs: number;
  jitterMs: number;
  throughputMbps: number;
  dropsQueueOverflow: number;
  dropsTtlExpired: number;
  dropsRadioLoss: number;
  dropsNoRoute: number;
}

export interface HandoverKPIs {
  totalAttempts: number;
  totalSuccesses: number;
  totalFailures: number;
  successRatePercent: number;
  pingPongCount: number;
  pingPongRatePercent: number;
  avgInterruptionMs: number;
}

export interface RadioKPIs {
  serviceAvailabilityPercent: number;
  outageProbabilityPercent: number;
  meanRsrpDbm: number;
  meanSinrDb: number;
  meanDopplerHz: number;
  maxDopplerHz: number;
}

export interface SimulationMetrics {
  simTime: number;
  traffic?: TrafficKPIs;
  handovers?: HandoverKPIs;
  radio?: RadioKPIs;
}

export interface ServerStatus {
  status: 'RUNNING' | 'STOPPED' | 'PAUSED' | 'OFFLINE';
  port: number;
  controllerConnected: boolean;
  isMockMode?: boolean;
}

export interface CommandResponse {
  success: boolean;
  status?: string;
  action?: string;
  source?: string;
  target?: string;
  bidirectional?: boolean;
  error?: string;
}

export interface LinkFaultPayload {
  source: string;
  target: string;
  bidirectional?: boolean;
}
