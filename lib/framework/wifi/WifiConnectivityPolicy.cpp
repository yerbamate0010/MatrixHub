#include "WifiConnectivityPolicy.h"

namespace WIFI_CONNECTIVITY_POLICY {

WiFiConnectivityPolicyDecision evaluate(const WiFiConnectivityPolicyInput& input) {
    WiFiConnectivityPolicyDecision decision = {};

    if (input.manualApOnly || !input.hasConfiguredNetworks) {
        decision.nextState = WiFiConnectivityState::ManualApOnly;
        return decision;
    }

    if (input.staConnected) {
        if (input.rescueApActive) {
            const bool stableLongEnough =
                input.stableConnectedSinceMs != 0 &&
                (input.nowMs - input.stableConnectedSinceMs) >= RESCUE_AP_EXIT_MS;
            if (stableLongEnough) {
                decision.exitRescueAp = true;
                decision.nextState = WiFiConnectivityState::StaConnected;
                return decision;
            }

            decision.nextState = WiFiConnectivityState::RescueApSta;
            return decision;
        }

        decision.nextState = WiFiConnectivityState::StaConnected;
        return decision;
    }

    if (input.rescueApActive) {
        decision.nextState = WiFiConnectivityState::RescueApSta;
        return decision;
    }

    const bool offlineLongEnough =
        input.disconnectedSinceMs != 0 &&
        (input.nowMs - input.disconnectedSinceMs) >= RESCUE_AP_ENTER_MS;
    if (input.completedNetworkCycle && offlineLongEnough) {
        decision.enterRescueAp = true;
        decision.nextState = WiFiConnectivityState::RescueApSta;
        return decision;
    }

    decision.nextState = WiFiConnectivityState::StaConnecting;
    return decision;
}

const char* stateName(WiFiConnectivityState state) {
    switch (state) {
        case WiFiConnectivityState::StaConnecting:
            return "sta_connecting";
        case WiFiConnectivityState::StaConnected:
            return "sta_connected";
        case WiFiConnectivityState::RescueApSta:
            return "rescue_ap_sta";
        case WiFiConnectivityState::ManualApOnly:
            return "manual_ap_only";
        default:
            return "unknown";
    }
}

}  // namespace WIFI_CONNECTIVITY_POLICY
