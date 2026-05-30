#include <unity.h>

#include "../../lib/framework/wifi/WifiConnectivityPolicy.h"
#include "../../lib/framework/wifi/WifiConnectivityPolicy.cpp"

using namespace WIFI_CONNECTIVITY_POLICY;

void test_enters_rescue_after_full_cycle_and_timeout() {
    WiFiConnectivityPolicyInput input = {};
    input.hasConfiguredNetworks = true;
    input.completedNetworkCycle = true;
    input.disconnectedSinceMs = 1000;
    input.nowMs = 1000 + RESCUE_AP_ENTER_MS + 1;

    const auto decision = evaluate(input);

    TEST_ASSERT_TRUE(decision.enterRescueAp);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(WiFiConnectivityState::RescueApSta),
                          static_cast<int>(decision.nextState));
}

void test_does_not_enter_rescue_without_full_cycle() {
    WiFiConnectivityPolicyInput input = {};
    input.hasConfiguredNetworks = true;
    input.completedNetworkCycle = false;
    input.disconnectedSinceMs = 1000;
    input.nowMs = 1000 + RESCUE_AP_ENTER_MS + 1;

    const auto decision = evaluate(input);

    TEST_ASSERT_FALSE(decision.enterRescueAp);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(WiFiConnectivityState::StaConnecting),
                          static_cast<int>(decision.nextState));
}

void test_exits_rescue_after_stable_connection_window() {
    WiFiConnectivityPolicyInput input = {};
    input.hasConfiguredNetworks = true;
    input.staConnected = true;
    input.rescueApActive = true;
    input.stableConnectedSinceMs = 500;
    input.nowMs = 500 + RESCUE_AP_EXIT_MS + 1;

    const auto decision = evaluate(input);

    TEST_ASSERT_TRUE(decision.exitRescueAp);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(WiFiConnectivityState::StaConnected),
                          static_cast<int>(decision.nextState));
}

void test_manual_ap_only_is_sticky() {
    WiFiConnectivityPolicyInput input = {};
    input.hasConfiguredNetworks = true;
    input.manualApOnly = true;
    input.completedNetworkCycle = true;
    input.disconnectedSinceMs = 1000;
    input.nowMs = 1000 + RESCUE_AP_ENTER_MS + 1;

    const auto decision = evaluate(input);

    TEST_ASSERT_FALSE(decision.enterRescueAp);
    TEST_ASSERT_FALSE(decision.exitRescueAp);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(WiFiConnectivityState::ManualApOnly),
                          static_cast<int>(decision.nextState));
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();
    RUN_TEST(test_enters_rescue_after_full_cycle_and_timeout);
    RUN_TEST(test_does_not_enter_rescue_without_full_cycle);
    RUN_TEST(test_exits_rescue_after_stable_connection_window);
    RUN_TEST(test_manual_ap_only_is_sticky);
    return UNITY_END();
}
