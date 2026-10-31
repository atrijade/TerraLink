import type {
  Satellite,
  TopologyLink,
  Gateway,
  User,
  SimulationMetrics,
  ServerStatus,
  CommandResponse,
} from './types';

// Global mock state
let mockSimRunning = true;
let mockSimStartTime = Date.now();
let mockSimTime = 0.0;
const severedLinks = new Set<string>();

const NUM_PLANES = 4;
const SATS_PER_PLANE = 6;
const ORBIT_ALTITUDE_KM = 600;
const ORBIT_INCLINATION_RAD = (53.0 * Math.PI) / 180.0;
const EARTH_RADIUS_KM = 6371.0;
const ORBIT_PERIOD_SEC = 5700.0; // ~95 minutes orbital period

function makeLinkId(src: string, dst: string): string {
  return [src, dst].sort().join('---');
}

/**
 * Calculates satellite positions for a Walker constellation at simulated time t (seconds).
 */
export function generateMockSatellites(tSec: number): Satellite[] {
  const satellites: Satellite[] = [];
  const meanMotion = (2 * Math.PI) / ORBIT_PERIOD_SEC;

  for (let p = 0; p < NUM_PLANES; p++) {
    const raan = (p * 2 * Math.PI) / NUM_PLANES;
    const planePhaseOffset = (p * (2 * Math.PI / NUM_PLANES) * 0.5);

    for (let s = 0; s < SATS_PER_PLANE; s++) {
      const meanAnomaly = (s * 2 * Math.PI) / SATS_PER_PLANE + planePhaseOffset + meanMotion * tSec;
      const satId = `SAT_P${p}_S${s}`;

      // Orbital coordinates in orbital plane
      const r = EARTH_RADIUS_KM + ORBIT_ALTITUDE_KM;
      const xOrb = r * Math.cos(meanAnomaly);
      const yOrb = r * Math.sin(meanAnomaly);

      // Rotate by inclination and RAAN to inertial/ECEF
      const xEci = xOrb * Math.cos(raan) - yOrb * Math.cos(ORBIT_INCLINATION_RAD) * Math.sin(raan);
      const yEci = xOrb * Math.sin(raan) + yOrb * Math.cos(ORBIT_INCLINATION_RAD) * Math.cos(raan);
      const zEci = yOrb * Math.sin(ORBIT_INCLINATION_RAD);

      // Account for Earth's rotation
      const earthRotAngle = (tSec * (2 * Math.PI / 86400.0)) % (2 * Math.PI);
      const xEcef = xEci * Math.cos(earthRotAngle) + yEci * Math.sin(earthRotAngle);
      const yEcef = -xEci * Math.sin(earthRotAngle) + yEci * Math.cos(earthRotAngle);
      const zEcef = zEci;

      // Convert ECEF to geodetic lat/lon
      const pDist = Math.sqrt(xEcef * xEcef + yEcef * yEcef);
      const latRad = Math.atan2(zEcef, pDist);
      const lonRad = Math.atan2(yEcef, xEcef);

      const lat = (latRad * 180.0) / Math.PI;
      const lon = (lonRad * 180.0) / Math.PI;

      // Orbital velocity (~7.5 km/s)
      const vMag = 7.56;
      const vx = -vMag * Math.sin(meanAnomaly);
      const vy = vMag * Math.cos(meanAnomaly);

      satellites.push({
        id: satId,
        lat,
        lon,
        altKm: ORBIT_ALTITUDE_KM,
        altitudeKm: ORBIT_ALTITUDE_KM,
        ecef: { x: xEcef * 1000, y: yEcef * 1000, z: zEcef * 1000 },
        velKms: { x: vx, y: vy, z: 0 },
        orbitPlane: p,
      });
    }
  }

  return satellites;
}

/**
 * Generates ISL links (Intra-plane and Inter-plane mesh grid).
 */
export function generateMockTopology(satellites: Satellite[]): { links: TopologyLink[] } {
  const links: TopologyLink[] = [];
  const satMap = new Map<string, Satellite>();
  satellites.forEach((s) => satMap.set(s.id, s));

  for (let p = 0; p < NUM_PLANES; p++) {
    for (let s = 0; s < SATS_PER_PLANE; s++) {
      const srcId = `SAT_P${p}_S${s}`;

      // 1. Intra-plane neighbor (ahead in same plane)
      const nextSatInPlane = (s + 1) % SATS_PER_PLANE;
      const intraDstId = `SAT_P${p}_S${nextSatInPlane}`;
      const intraLinkId = makeLinkId(srcId, intraDstId);
      const isIntraActive = !severedLinks.has(intraLinkId);

      links.push({
        src: srcId,
        dst: intraDstId,
        distanceKm: 2850.0 + Math.sin(s) * 20.0,
        delayMs: 9.5 + Math.random() * 0.4,
        dataRateGbps: 10.0,
        active: isIntraActive,
        isInterPlane: false,
        type: 'ISL',
        status: isIntraActive ? 'ACTIVE' : 'FAILED',
      });

      // 2. Inter-plane neighbor (adjacent plane)
      const nextPlane = (p + 1) % NUM_PLANES;
      const interDstId = `SAT_P${nextPlane}_S${s}`;
      const interLinkId = makeLinkId(srcId, interDstId);
      const isInterActive = !severedLinks.has(interLinkId);

      links.push({
        src: srcId,
        dst: interDstId,
        distanceKm: 3400.0 + Math.cos(p) * 50.0,
        delayMs: 11.3 + Math.random() * 0.5,
        dataRateGbps: 10.0,
        active: isInterActive,
        isInterPlane: true,
        type: 'ISL',
        status: isInterActive ? 'ACTIVE' : 'FAILED',
      });
    }
  }

  return { links };
}

/**
 * Mock Ground Gateways
 */
export function generateMockGateways(satellites: Satellite[]): Gateway[] {
  const baseGateways = [
    { id: 'GW_SVALBARD', lat: 78.22, lon: 15.65, name: 'Svalbard' },
    { id: 'GW_FRANKFURT', lat: 50.11, lon: 8.68, name: 'Frankfurt' },
    { id: 'GW_HAWAII', lat: 21.31, lon: -157.86, name: 'Hawaii' },
    { id: 'GW_SINGAPORE', lat: 1.35, lon: 103.82, name: 'Singapore' },
  ];

  return baseGateways.map((gw, i) => {
    // Find closest satellite
    let closestSat = satellites[0]?.id || 'SAT_P0_S0';
    let minDist = Infinity;

    for (const sat of satellites) {
      const dLat = sat.lat - gw.lat;
      const dLon = sat.lon - gw.lon;
      const dist = dLat * dLat + dLon * dLon;
      if (dist < minDist) {
        minDist = dist;
        closestSat = sat.id;
      }
    }

    return {
      id: gw.id,
      lat: gw.lat,
      lon: gw.lon,
      altM: 50.0,
      connectedSat: closestSat,
      hasActiveFeeder: true,
      throughputMbps: 450.0 + (i * 45) + Math.sin(Date.now() / 3000) * 30,
      jitterMs: 0.8 + Math.random() * 0.4,
      avgLatencyMs: 18.2 + Math.random() * 2.1,
      packetsReceived: 245000 + Math.floor(Math.random() * 500),
      packetsLost: severedLinks.size > 0 ? 14 + Math.floor(Math.random() * 6) : 2,
      packetsOutOfOrder: 1,
      utilizationPercent: 62.5 + Math.random() * 8.0,
      feederProfile: {
        carrierFreqGhz: 28.5,
        bandwidthMhz: 400.0,
        dataRateGbps: 2.5,
        minElevationDeg: 10.0,
      },
    };
  });
}

/**
 * Mock Ground User Terminals
 */
export function generateMockUsers(satellites: Satellite[]): User[] {
  const baseUsers = [
    { id: 'UE_LONDON', lat: 51.5074, lon: -0.1278 },
    { id: 'UE_NEW_YORK', lat: 40.7128, lon: -74.006 },
    { id: 'UE_TOKYO', lat: 35.6762, lon: 139.6503 },
    { id: 'UE_SYDNEY', lat: -33.8688, lon: 151.2093 },
    { id: 'UE_SAO_PAULO', lat: -23.5505, lon: -46.6333 },
    { id: 'UE_MUMBAI', lat: 19.076, lon: 72.8777 },
    { id: 'UE_NAIROBI', lat: -1.2921, lon: 36.8219 },
    { id: 'UE_REYKJAVIK', lat: 64.1466, lon: -21.9426 },
  ];

  return baseUsers.map((u) => {
    // Pick nearest satellite
    let nearestSat = satellites[0]?.id || 'SAT_P0_S0';
    let minDist = Infinity;
    for (const sat of satellites) {
      const dLat = sat.lat - u.lat;
      const dLon = sat.lon - u.lon;
      const dist = dLat * dLat + dLon * dLon;
      if (dist < minDist) {
        minDist = dist;
        nearestSat = sat.id;
      }
    }

    return {
      id: u.id,
      lat: u.lat,
      lon: u.lon,
      altM: 10.0,
      servingSat: nearestSat,
      trafficRatePps: 150 + Math.floor(Math.random() * 50),
      sinrDb: 18.5 + Math.random() * 3.0,
      rsrpDbm: -88.0 + Math.random() * 4.0,
    };
  });
}

/**
 * Mock Simulation Metrics & KPIs
 */
export function generateMockMetrics(): SimulationMetrics {
  const isDegraded = severedLinks.size > 0;
  const elapsed = mockSimRunning ? (Date.now() - mockSimStartTime) / 1000 : mockSimTime;
  mockSimTime = elapsed;

  const baseLatency = isDegraded ? 38.4 : 24.2;
  const pdr = isDegraded ? 95.8 : 99.2;

  return {
    simTime: mockSimTime,
    traffic: {
      packetsSent: 120000 + Math.floor(mockSimTime * 450),
      packetsDelivered: 119000 + Math.floor(mockSimTime * 440),
      packetsDropped: isDegraded ? 480 + Math.floor(mockSimTime * 10) : 45,
      pdrPercent: pdr + Math.sin(Date.now() / 2000) * 0.3,
      avgLatencyMs: baseLatency + Math.sin(Date.now() / 1500) * 1.5,
      medianLatencyMs: baseLatency - 2.0,
      p95LatencyMs: baseLatency * 1.45,
      p99LatencyMs: baseLatency * 1.85,
      jitterMs: isDegraded ? 4.2 : 1.4,
      throughputMbps: 840.0 + Math.cos(Date.now() / 3000) * 40,
      dropsQueueOverflow: isDegraded ? 180 : 12,
      dropsTtlExpired: isDegraded ? 90 : 5,
      dropsRadioLoss: 15,
      dropsNoRoute: isDegraded ? 195 : 0,
    },
    handovers: {
      totalAttempts: 142 + Math.floor(mockSimTime * 0.8),
      totalSuccesses: 139 + Math.floor(mockSimTime * 0.78),
      totalFailures: 3,
      successRatePercent: 97.9,
      pingPongCount: 4,
      pingPongRatePercent: 2.8,
      avgInterruptionMs: 14.6,
    },
    radio: {
      serviceAvailabilityPercent: 99.8,
      outageProbabilityPercent: 0.2,
      meanRsrpDbm: -89.4,
      meanSinrDb: 19.2,
      meanDopplerHz: 18450.0 + Math.sin(Date.now() / 1000) * 2000,
      maxDopplerHz: 34200.0,
    },
  };
}

export function getMockStatus(): ServerStatus {
  return {
    status: mockSimRunning ? 'RUNNING' : 'STOPPED',
    port: 8080,
    controllerConnected: true,
    isMockMode: true,
  };
}

export function startMockSimulation(): CommandResponse {
  mockSimRunning = true;
  mockSimStartTime = Date.now() - mockSimTime * 1000;
  return { success: true, status: 'STARTED' };
}

export function stopMockSimulation(): CommandResponse {
  mockSimRunning = false;
  return { success: true, status: 'STOPPED' };
}

export function injectMockLinkFailure(source: string, target: string, bidirectional = true): CommandResponse {
  const linkId = makeLinkId(source, target);
  severedLinks.add(linkId);
  return {
    success: true,
    action: 'LINK_FAILURE_INJECTED',
    source,
    target,
    bidirectional,
  };
}

export function restoreMockLink(source: string, target: string, bidirectional = true): CommandResponse {
  const linkId = makeLinkId(source, target);
  severedLinks.delete(linkId);
  return {
    success: true,
    action: 'LINK_RESTORED',
    source,
    target,
    bidirectional,
  };
}

export function getSeveredLinks(): Set<string> {
  return new Set(severedLinks);
}
