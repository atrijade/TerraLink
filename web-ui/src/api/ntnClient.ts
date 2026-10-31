import type {
  Satellite,
  TopologyResponse,
  Gateway,
  User,
  SimulationMetrics,
  ServerStatus,
  CommandResponse,
} from './types';
import {
  generateMockSatellites,
  generateMockTopology,
  generateMockGateways,
  generateMockUsers,
  generateMockMetrics,
  getMockStatus,
  startMockSimulation,
  stopMockSimulation,
  injectMockLinkFailure,
  restoreMockLink,
} from './mockData';

class NtnClient {
  private baseUrl: string = '';
  private forceMock: boolean = false;
  private backendAvailable: boolean = true;
  private consecutiveFailures: number = 0;
  private lastMockTimeSec: number = 0;

  constructor(baseUrl: string = '') {
    this.baseUrl = baseUrl;
  }

  public setForceMock(force: boolean): void {
    this.forceMock = force;
  }

  public isMockActive(): boolean {
    return this.forceMock || !this.backendAvailable;
  }

  private getMockSimTime(): number {
    this.lastMockTimeSec += 0.5;
    return this.lastMockTimeSec;
  }

  private async request<T>(endpoint: string, options?: RequestInit): Promise<T> {
    if (this.forceMock) {
      throw new Error('Forced mock mode enabled');
    }

    const controller = new AbortController();
    const timeoutId = setTimeout(() => controller.abort(), 2000);

    try {
      const url = `${this.baseUrl}${endpoint}`;
      const response = await fetch(url, {
        ...options,
        signal: controller.signal,
        headers: {
          'Content-Type': 'application/json',
          ...(options?.headers || {}),
        },
      });

      clearTimeout(timeoutId);

      if (!response.ok) {
        throw new Error(`HTTP error ${response.status}: ${response.statusText}`);
      }

      const data = await response.json();
      this.backendAvailable = true;
      this.consecutiveFailures = 0;
      return data as T;
    } catch (err) {
      clearTimeout(timeoutId);
      this.consecutiveFailures++;
      if (this.consecutiveFailures >= 2) {
        this.backendAvailable = false;
      }
      throw err;
    }
  }

  // =========================================================================
  // Telemetry Fetchers
  // =========================================================================

  public async fetchSatellites(): Promise<Satellite[]> {
    try {
      const data = await this.request<Satellite[]>('/satellites');
      if (Array.isArray(data) && data.length > 0) {
        return data;
      }
      // If server returned empty array (simulation not yet started or empty)
      if (this.isMockActive() || (Array.isArray(data) && data.length === 0)) {
        return generateMockSatellites(this.getMockSimTime());
      }
      return data;
    } catch {
      return generateMockSatellites(this.getMockSimTime());
    }
  }

  public async fetchTopology(): Promise<TopologyResponse> {
    try {
      const data = await this.request<TopologyResponse>('/topology');
      if (data && Array.isArray(data.links) && data.links.length > 0) {
        return data;
      }
      const sats = generateMockSatellites(this.lastMockTimeSec);
      return generateMockTopology(sats);
    } catch {
      const sats = generateMockSatellites(this.lastMockTimeSec);
      return generateMockTopology(sats);
    }
  }

  public async fetchGateways(): Promise<Gateway[]> {
    try {
      const data = await this.request<Gateway[]>('/gateways');
      if (Array.isArray(data) && data.length > 0) {
        return data;
      }
      const sats = generateMockSatellites(this.lastMockTimeSec);
      return generateMockGateways(sats);
    } catch {
      const sats = generateMockSatellites(this.lastMockTimeSec);
      return generateMockGateways(sats);
    }
  }

  public async fetchUsers(): Promise<User[]> {
    try {
      const data = await this.request<User[]>('/users');
      if (Array.isArray(data) && data.length > 0) {
        return data;
      }
      const sats = generateMockSatellites(this.lastMockTimeSec);
      return generateMockUsers(sats);
    } catch {
      const sats = generateMockSatellites(this.lastMockTimeSec);
      return generateMockUsers(sats);
    }
  }

  public async fetchMetrics(): Promise<SimulationMetrics> {
    try {
      return await this.request<SimulationMetrics>('/metrics');
    } catch {
      return generateMockMetrics();
    }
  }

  public async fetchStatus(): Promise<ServerStatus> {
    try {
      const res = await this.request<ServerStatus>('/status');
      return {
        ...res,
        isMockMode: false,
      };
    } catch {
      return getMockStatus();
    }
  }

  // =========================================================================
  // Control Mutations
  // =========================================================================

  public async startSimulation(): Promise<CommandResponse> {
    try {
      return await this.request<CommandResponse>('/start', { method: 'POST' });
    } catch {
      return startMockSimulation();
    }
  }

  public async stopSimulation(): Promise<CommandResponse> {
    try {
      return await this.request<CommandResponse>('/stop', { method: 'POST' });
    } catch {
      return stopMockSimulation();
    }
  }

  public async injectLinkFailure(
    source: string,
    target: string,
    bidirectional: boolean = true
  ): Promise<CommandResponse> {
    try {
      return await this.request<CommandResponse>('/api/inject/link-failure', {
        method: 'POST',
        body: JSON.stringify({ source, target, bidirectional }),
      });
    } catch {
      return injectMockLinkFailure(source, target, bidirectional);
    }
  }

  public async restoreLink(
    source: string,
    target: string,
    bidirectional: boolean = true
  ): Promise<CommandResponse> {
    try {
      return await this.request<CommandResponse>('/api/inject/link-restore', {
        method: 'POST',
        body: JSON.stringify({ source, target, bidirectional }),
      });
    } catch {
      return restoreMockLink(source, target, bidirectional);
    }
  }
}

export const ntnClient = new NtnClient();
