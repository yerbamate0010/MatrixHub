import { createDeviceApiClient, type DeviceApiClientOptions } from "./api/client";
import type { ApSettings, ApStatus, WifiSettings, WifiStatus } from "./types";

function stripUndefined<T extends Record<string, unknown>>(value: T) {
  const result: Record<string, unknown> = {};
  Object.entries(value).forEach(([key, entryValue]) => {
    if (entryValue !== undefined) {
      result[key] = entryValue;
    }
  });
  return result;
}

export class DeviceNetworkApi {
  private client;

  constructor(options: DeviceApiClientOptions) {
    this.client = createDeviceApiClient(options);
  }

  async getWifiStatus(): Promise<WifiStatus> {
    return this.client.get<WifiStatus>("/rest/wifiStatus");
  }

  async getWifiSettings(): Promise<WifiSettings> {
    return this.client.get<WifiSettings>("/rest/wifiSettings");
  }

  async saveWifiSettings(settings: WifiSettings): Promise<WifiSettings> {
    const payload = {
      ...settings,
      hostname: settings.hostname || "matrixhub",
      wifi_networks: settings.wifi_networks.map((network) =>
        stripUndefined(network as unknown as Record<string, unknown>)
      )
    };

    return this.client.post<WifiSettings>("/rest/wifiSettings", payload);
  }

  async getApStatus(): Promise<ApStatus> {
    return this.client.get<ApStatus>("/rest/apStatus");
  }

  async getApSettings(): Promise<ApSettings> {
    return this.client.get<ApSettings>("/rest/apSettings");
  }
}
