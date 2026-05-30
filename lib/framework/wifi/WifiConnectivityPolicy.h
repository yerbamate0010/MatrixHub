#pragma once

#include <cstdint>

enum class WiFiConnectivityState : uint8_t {
    StaConnecting = 0,
    StaConnected,
    RescueApSta,
    ManualApOnly,
};

struct WiFiConnectivityPolicyInput {
    bool hasConfiguredNetworks = false;
    bool staConnected = false;
    bool rescueApActive = false;
    bool manualApOnly = false;
    bool completedNetworkCycle = false;
    uint32_t nowMs = 0;
    uint32_t disconnectedSinceMs = 0;
    uint32_t stableConnectedSinceMs = 0;
};

struct WiFiConnectivityPolicyDecision {
    bool enterRescueAp = false;
    bool exitRescueAp = false;
    WiFiConnectivityState nextState = WiFiConnectivityState::StaConnecting;
};

namespace WIFI_CONNECTIVITY_POLICY {

// Policy timing is intentionally asymmetric:
// - after prolonged outage we enter rescue AP_STA to restore field access even
//   if STA cannot recover on its own
// - after reconnect we wait for a shorter but still stable window before
//   removing rescue AP, so a flaky upstream network does not cause AP_STA <-> STA
//   flapping
constexpr uint32_t RESCUE_AP_ENTER_MS = 180000UL;
constexpr uint32_t RESCUE_AP_EXIT_MS = 120000UL;

WiFiConnectivityPolicyDecision evaluate(const WiFiConnectivityPolicyInput& input);
const char* stateName(WiFiConnectivityState state);

}  // namespace WIFI_CONNECTIVITY_POLICY
