export type StorageMetricsInfo = {
  backend: string;
  available: boolean;
  mounted: boolean;
  total_bytes: number;
  used_bytes: number;
  free_bytes: number;
  last_error?: string;
};

export type SystemStorageInfo = {
  filesystem: { total_bytes: number; used_bytes: number; free_bytes: number };
  active_backend: string;
  active_path: string;
  active: StorageMetricsInfo;
  littlefs: StorageMetricsInfo;
  sdcard: StorageMetricsInfo;
};

export type SystemInformation = {
  esp_platform: string;
  firmware_version: string;
  firmware_name?: string;
  firmware_built_target?: string;
  cpu_freq_mhz: number;
  cpu_type: string;
  cpu_rev: number;
  cpu_cores: number;
  sketch_size: number;
  free_sketch_space: number;
  sdk_version: string;
  arduino_version: string;
  flash_chip_size: number;
  flash_chip_speed: number;
  cpu_reset_reason: number;
  max_alloc_heap: number;
  psram_size: number;
  free_psram: number;
  used_psram: number;
  free_heap: number;
  used_heap: number;
  total_heap: number;
  min_free_heap: number;
  core_temp: number;
  fs_total: number;
  fs_used: number;
  lp_sram_used?: number;
  lp_sram_free?: number;
  lp_sram_total?: number;
  uptime: number;
  mac_address?: string;
  compile_date?: string;
  compile_time?: string;
  storage?: SystemStorageInfo;
};

export interface SystemStatus {
  timestamp: number;
  lastUpdate: number;
  wifiStatus: number;
  rssi: number;
  isConnected: boolean;
  isStaConnected?: boolean;
  isApMode?: boolean;
  coreTemp?: number;
}

export const DEFAULT_SYSTEM_STATUS: SystemStatus = {
  timestamp: 0,
  lastUpdate: 0,
  wifiStatus: 0,
  rssi: 0,
  isConnected: false,
  isStaConnected: false,
  isApMode: false,
  coreTemp: 0,
};

export interface SystemStatusWifiDiagnostics {
  connected?: boolean;
  lastDisconnectReason?: number;
  healthy?: boolean;
  state?: string;
  mode?: string;
  rescueApActive?: boolean;
  rescueReason?: string;
  lastRecoveryReason?: string;
  lastIpChangeMs?: number;
  disconnectedSinceMs?: number;
  stableConnectedSinceMs?: number;
  rssi?: number;
  ssid?: string;
  ip?: string;
  savedStaticIp?: string;
  mac?: string;
  channel?: number;
  bssid?: string;
  gateway?: string;
  subnet?: string;
  dns?: string;
}

export interface SystemStatusApDiagnostics {
  active?: boolean;
  mode?: string;
  stationNum?: number;
  ip?: string;
  mac?: string;
}

export interface SystemStatusHttpDiagnostics {
  activeClients?: number;
  peakClients?: number;
  opens?: number;
  closes?: number;
  lastOpenMs?: number;
  lastCloseMs?: number;
  wsActiveClients?: number;
  wsPeakClients?: number;
  wsOpens?: number;
  wsCloses?: number;
  lastWsOpenMs?: number;
  lastWsCloseMs?: number;
  wsForcedRemovals?: number;
  wsQueueDrops?: number;
  lastWsQueueDropMs?: number;
  lastWsQueueDropPayload?: number;
  wsHeapFallbacks?: number;
  lastWsHeapFallbackMs?: number;
  lastWsHeapFallbackPayload?: number;
  maxWsHeapFallbackPayload?: number;
}

export interface SystemStatusForwardingDiagnostics {
  ready?: boolean;
  requiresStaticIp?: boolean;
  savedStaticIpConfigured?: boolean;
  savedStaticIpMatches?: boolean;
  httpsPort?: number;
}

export interface DashboardBleWidgetSummary {
  enabled: boolean;
  sensor_count: number;
}

export interface DashboardShellyWidgetSummary {
  device_count: number;
}

export interface DashboardAlarmsWidgetSummary {
  rule_count: number;
}

export interface DashboardWifiSensingWidgetSummary {
  enabled: boolean;
}

export interface SystemStatusDashboardWidgetsSummary {
  ble: DashboardBleWidgetSummary;
  shelly: DashboardShellyWidgetSummary;
  alarms: DashboardAlarmsWidgetSummary;
  wifi_sensing: DashboardWifiSensingWidgetSummary;
}

export interface SystemStatusSnapshot {
  system_info?: SystemInformation;
  diagnostics?: {
    wifi?: SystemStatusWifiDiagnostics;
    ap?: SystemStatusApDiagnostics;
    http?: SystemStatusHttpDiagnostics;
    forwarding?: SystemStatusForwardingDiagnostics;
  };
  dashboard_widgets?: SystemStatusDashboardWidgetsSummary;
  wifi_ap_mode?: boolean;
  wifi_settings?: {
    connection_mode?: number;
    wifi_networks?: Array<{ ssid: string }>;
  };
  config?: {
    logging?: {
      level?: string;
    };
  };
}

export interface TelemetryHistorySnapshot {
  timestamps?: number[];
  co2?: Array<number | null>;
  temp?: Array<number | null>;
  humid?: Array<number | null>;
}

export interface TelemetrySnapshot {
  co2: number;
  temp: number;
  humid: number;
  lastReadOk?: boolean;
  history?: TelemetryHistorySnapshot;
}

export interface SensorTelemetryEvent {
  co2: number;
  temp: number;
  humid: number;
  lastReadOk: boolean;
  timestamp_ms: number;
}

export type SnapshotChannel =
  | "shelly"
  | "alarms"
  | "ble"
  | "sensing"
  | "telemetry"
  | "system_status"
  | "airmouse"
  | "telegram"
  | "notif_stats";

export interface BleSensorConfig {
  mac: string;
  alias: string;
}

export interface BleSettings {
  enabled: boolean;
  sensors?: BleSensorConfig[];
}

export interface BleDevice {
  mac: string;
  temp: number;
  humid: number;
  batt: number;
  rssi: number;
  last_seen: number;
  saved?: boolean;
}

export interface BleStatus {
  enabled: boolean;
  running: boolean;
  scanner_active?: boolean;
  metrics?: {
    adv_total: number;
    valid_packets: number;
    parser_errors: number;
    cache_drops: number;
    mutex_timeouts: number;
    scanner_running: boolean;
  };
  settings?: BleSettings;
  devices?: BleDevice[];
}

export interface BleDeviceEvent {
  mac: string;
  temp: number;
  humid: number;
  batt: number;
  rssi: number;
  lastSeen: number;
}

export interface ShellyDevice {
  id: string;
  name: string;
  isOn: boolean;
  isOnline: boolean;
  ip?: string;
  relayIndex?: number;
  enabled?: boolean;
  generation?: number;
  lastUpdate?: number;
  power?: number;
  energy?: number;
  voltage?: number;
  current?: number;
  temp?: number;
  rssi?: number;
}

export interface ShellyDeviceEvent {
  id: string;
  isOn: boolean;
  isOnline: boolean;
  power?: number;
  energy?: number;
  voltage?: number;
  current?: number;
  temp?: number;
  rssi?: number;
}

export interface SignInRequest {
  username: string;
  password: string;
}

export interface SignInResponse {
  access_token: string;
}

export interface AccessTokenPayload {
  username?: string;
  admin?: boolean;
  iat?: number;
}

export interface DeviceRecord {
  id: string;
  name: string;
  origin: string;
  input: string;
  createdAt: string;
  lastConnectedAt?: string;
}

export interface DeviceSession {
  accessToken: string;
  username: string;
  admin: boolean;
  signedInAt: string;
}

export interface LoggingConfig {
  level: string;
}

export interface AppConfig {
  logging?: LoggingConfig;
  [key: string]: unknown;
}

export interface WifiRecoveryResponse {
  success: boolean;
  accepted: boolean;
  connected: boolean;
  ip?: string;
  rssi?: number;
}

export interface TaskInfo {
  name: string;
  priority: number;
  stackHighWaterMark: number;
  state: string;
  coreId?: number;
}

export interface TasksResponse {
  watchdog: {
    initialized: boolean;
    timeoutSec: number;
  };
  taskCount: number;
  detailsIncluded?: boolean;
  tasks?: TaskInfo[];
  error?: string;
  memory: {
    freeHeap: number;
    minFreeHeap: number;
    freePsram?: number;
  };
}

export interface SystemNetworkInfo {
  wifi: {
    state: string;
    configured_mode: string;
    mode: string;
    mode_id: number;
    sta_connected: boolean;
    ap_active: boolean;
    last_disconnect_reason: number;
    last_ip_change_ms: number;
    disconnected_since_ms: number;
    stable_connected_since_ms: number;
    last_recovery_reason: string;
    sta_ip?: string;
    saved_static_ip?: string;
  };
  ap: {
    active: boolean;
    station_num: number;
    ip?: string;
  };
  http: {
    active_clients: number;
    peak_clients: number;
    opens: number;
    closes: number;
    last_open_ms: number;
    last_close_ms: number;
    ws_active_clients: number;
    ws_peak_clients: number;
    ws_opens: number;
    ws_closes: number;
    last_ws_open_ms: number;
    last_ws_close_ms: number;
    ws_forced_removals: number;
    ws_queue_drops: number;
    last_ws_queue_drop_ms: number;
    last_ws_queue_drop_payload: number;
    ws_heap_fallbacks: number;
    last_ws_heap_fallback_ms: number;
    last_ws_heap_fallback_payload: number;
    max_ws_heap_fallback_payload: number;
  };
  forwarding: {
    ready: boolean;
    requires_static_ip: boolean;
    saved_static_ip_configured: boolean;
    saved_static_ip_matches: boolean;
    https_port: number;
  };
}

export interface WifiStatus {
  status: number;
  local_ip?: string;
  mac_address?: string;
  rssi?: number;
  ssid?: string;
  bssid?: string;
  channel?: number;
  subnet_mask?: string;
  gateway_ip?: string;
  dns_ip_1?: string;
  dns_ip_2?: string;
}

export type WifiMode = "off" | "ap" | "sta";

export interface KnownNetworkItem {
  ssid: string;
  password: string;
  static_ip_config: boolean;
  local_ip?: string;
  subnet_mask?: string;
  gateway_ip?: string;
  dns_ip_1?: string;
  dns_ip_2?: string;
}

export interface WifiSettings {
  hostname: string;
  mode: WifiMode;
  /** Legacy websocket snapshot field; REST settings use `mode`. */
  connection_mode?: number;
  wifi_networks: KnownNetworkItem[];
}

export type NetworkScanState = "idle" | "running" | "ready";

export interface NetworkItem {
  rssi: number;
  ssid: string;
  bssid: string;
  channel: number;
  encryption_type: number;
}

export interface NetworkListResponse {
  networks: NetworkItem[];
  scan_state?: NetworkScanState;
}

export interface NtpSettings {
  enabled: boolean;
  server: string;
  tz_label: string;
  tz_format: string;
}

export interface NtpStatus {
  status: number;
  time_valid?: boolean;
  utc_time: string;
  local_time: string;
  server: string;
  uptime: number;
}

export interface ApStatus {
  status: number;
  ip_address: string;
  mac_address: string;
  station_num: number;
}

export interface ApSettings {
  ssid: string;
  password: string;
  channel: number;
  ssid_hidden: boolean;
  max_clients: number;
  local_ip: string;
  gateway_ip: string;
  subnet_mask: string;
}
