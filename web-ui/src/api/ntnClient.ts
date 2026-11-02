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
  setMockSpeedMultiplier,
  getMockSpeedMultiplier,
  resetMockSimulationTime,
  isMockSimRunning,
} from './mockData';

class NtnClient {
  private baseUrl: string = '';
  private enableMockFallback: boolean = false;
  private backendAvailable: boolean = true;
  private simRunning: boolean = true;

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

  public isSimulationRunning(): boolean {
    if (this.enableMockFallback) {
      return isMockSimRunning();
    }
    return this.simRunning;
  }

  public setSpeedMultiplier(multiplier: number): void {
    setMockSpeedMultiplier(multiplier);
  }

  public getSpeedMultiplier(): number {
    return getMockSpeedMultiplier();
  }

  public resetSimulationTime(): void {
    resetMockSimulationTime();
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
      if (this.enableMockFallback || !this.backendAvailable) {
        return generateMockSatellites();
      }
      throw err;
    }
  }

  public async fetchTopology(): Promise<TopologyResponse> {
    try {
      const data = await this.request<TopologyResponse>('/topology');
      return data && Array.isArray(data.links) ? data : { links: [] };
    } catch (err) {
      if (this.enableMockFallback || !this.backendAvailable) {
        const sats = generateMockSatellites();
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
      if (this.enableMockFallback || !this.backendAvailable) {
        const sats = generateMockSatellites();
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
      if (this.enableMockFallback || !this.backendAvailable) {
        const sats = generateMockSatellites();
        return generateMockUsers(sats);
      }
      throw err;
    }
  }

  public async fetchMetrics(): Promise<SimulationMetrics> {
    try {
      return await this.request<SimulationMetrics>('/metrics');
    } catch (err) {
      if (this.enableMockFallback || !this.backendAvailable) {
        return generateMockMetrics();
      }
      throw err;
    }
  }

  public async fetchStatus(): Promise<ServerStatus> {
    try {
      const res = await this.request<ServerStatus>('/status');
      this.simRunning = res.status === 'RUNNING';
      return {
        ...res,
        isMockMode: false,
      };
    } catch (err) {
      const mockStatus = getMockStatus();
      this.simRunning = mockStatus.status === 'RUNNING';
      return {
        ...mockStatus,
        controllerConnected: false,
        isMockMode: true,
      };
    }
  }

  // =========================================================================
  // Control Mutations
  // =========================================================================

  public async startSimulation(): Promise<CommandResponse> {
    this.simRunning = true;
    startMockSimulation();
    try {
      return await this.request<CommandResponse>('/start', { method: 'POST' });
    } catch (err) {
      if (this.enableMockFallback || !this.backendAvailable) {
        return { success: true, status: 'STARTED' };
      }
      throw err;
    }
  }

  public async stopSimulation(): Promise<CommandResponse> {
    this.simRunning = false;
    stopMockSimulation();
    try {
      return await this.request<CommandResponse>('/stop', { method: 'POST' });
    } catch (err) {
      if (this.enableMockFallback || !this.backendAvailable) {
        return { success: true, status: 'STOPPED' };
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
