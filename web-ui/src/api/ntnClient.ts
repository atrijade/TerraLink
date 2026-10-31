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
  // Mock fallback is disabled per user request
  private enableMockFallback: boolean = false;
  private backendAvailable: boolean = true;
  private lastMockTimeSec: number = 0;

  constructor(baseUrl: string = '') {
    this.baseUrl = baseUrl;
  }

  public setEnableMockFallback(enable: boolean): void {
    this.enableMockFallback = enable;
  }

  public isMockFallbackEnabled(): boolean {
    return this.enableMockFallback;
  }

  public isBackendOnline(): boolean {
    return this.backendAvailable;
  }

  private getMockSimTime(): number {
    this.lastMockTimeSec += 0.5;
    return this.lastMockTimeSec;
  }

  private async request<T>(endpoint: string, options?: RequestInit): Promise<T> {
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
        throw new Error(`HTTP ${response.status}: ${response.statusText}`);
      }

      const data = await response.json();
      this.backendAvailable = true;
      return data as T;
    } catch (err) {
      clearTimeout(timeoutId);
      this.backendAvailable = false;
      throw err;
    }
  }

  // =========================================================================
  // Telemetry Fetchers
  // =========================================================================

  public async fetchSatellites(): Promise<Satellite[]> {
    try {
      const data = await this.request<Satellite[]>('/satellites');
      return Array.isArray(data) ? data : [];
    } catch (err) {
      if (this.enableMockFallback) {
        return generateMockSatellites(this.getMockSimTime());
      }
      throw err;
    }
  }

  public async fetchTopology(): Promise<TopologyResponse> {
    try {
      const data = await this.request<TopologyResponse>('/topology');
      return data && Array.isArray(data.links) ? data : { links: [] };
    } catch (err) {
      if (this.enableMockFallback) {
        const sats = generateMockSatellites(this.lastMockTimeSec);
        return generateMockTopology(sats);
      }
      throw err;
    }
  }

  public async fetchGateways(): Promise<Gateway[]> {
    try {
      const data = await this.request<Gateway[]>('/gateways');
      return Array.isArray(data) ? data : [];
    } catch (err) {
      if (this.enableMockFallback) {
        const sats = generateMockSatellites(this.lastMockTimeSec);
        return generateMockGateways(sats);
      }
      throw err;
    }
  }

  public async fetchUsers(): Promise<User[]> {
    try {
      const data = await this.request<User[]>('/users');
      return Array.isArray(data) ? data : [];
    } catch (err) {
      if (this.enableMockFallback) {
        const sats = generateMockSatellites(this.lastMockTimeSec);
        return generateMockUsers(sats);
      }
      throw err;
    }
  }

  public async fetchMetrics(): Promise<SimulationMetrics> {
    try {
      return await this.request<SimulationMetrics>('/metrics');
    } catch (err) {
      if (this.enableMockFallback) {
        return generateMockMetrics();
      }
      throw err;
    }
  }

  public async fetchStatus(): Promise<ServerStatus> {
    try {
      const res = await this.request<ServerStatus>('/status');
      return {
        ...res,
        isMockMode: false,
      };
    } catch (err) {
      if (this.enableMockFallback) {
        return getMockStatus();
      }
      return {
        status: 'OFFLINE',
        port: 8080,
        controllerConnected: false,
        isMockMode: false,
      };
    }
  }

  // =========================================================================
  // Control Mutations
  // =========================================================================

  public async startSimulation(): Promise<CommandResponse> {
    try {
      return await this.request<CommandResponse>('/start', { method: 'POST' });
    } catch (err) {
      if (this.enableMockFallback) {
        return startMockSimulation();
      }
      throw err;
    }
  }

  public async stopSimulation(): Promise<CommandResponse> {
    try {
      return await this.request<CommandResponse>('/stop', { method: 'POST' });
    } catch (err) {
      if (this.enableMockFallback) {
        return stopMockSimulation();
      }
      throw err;
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
    } catch (err) {
      if (this.enableMockFallback) {
        return injectMockLinkFailure(source, target, bidirectional);
      }
      throw err;
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
    } catch (err) {
      if (this.enableMockFallback) {
        return restoreMockLink(source, target, bidirectional);
      }
      throw err;
    }
  }
}

export const ntnClient = new NtnClient();
