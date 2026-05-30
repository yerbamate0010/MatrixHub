import { createDeviceApiClient, type DeviceApiClientOptions } from "./api/client";
import type { SystemInformation, WifiRecoveryResponse } from "./types";

export class DeviceSystemApi {
  private client;

  constructor(options: DeviceApiClientOptions) {
    this.client = createDeviceApiClient(options);
  }

  async getSystemInfo(): Promise<SystemInformation> {
    return this.client.get<SystemInformation>("/api/system/info");
  }

  async triggerWifiRecovery(): Promise<WifiRecoveryResponse> {
    return this.client.post<WifiRecoveryResponse>("/api/system/wifi/recover");
  }
}
