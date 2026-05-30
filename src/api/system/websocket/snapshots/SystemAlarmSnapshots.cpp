#include "SystemSnapshots.h"

#include "../../../../alarms/AlarmService.h"
#include "../../../../alarms/engine/AlarmEvaluator.h"
#include "../../../../alarms/types/AlarmTypes.h"
#include "../../../../alarms/utils/BleDataProvider.h"
#include "../../../../ble/BleService.h"
#include "../../../../config/System.h"
#include "../../../../sensors/runtime/SensorState.h"
#include "../../../../system/memory/PsramAllocator.h"
#include "../../../../system/utils/ScopeLock.h"
#include "../../../../wifisensing/WifiSensingService.h"

#include <ArduinoJson.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace API::SYSTEM_WS {

void sendAlarmsSnapshot(const SnapshotContext& ctx) {
    if (!ctx.alarmService) {
        return;
    }

    auto& manager = ctx.alarmService->getManager();
    std::vector<ALARMS::AlarmRule, SYSTEM::PsramAllocator<ALARMS::AlarmRule>> rulesCopy;
    std::vector<ALARMS::AlarmRuntimeState, SYSTEM::PsramAllocator<ALARMS::AlarmRuntimeState>>
        statesCopy;
    {
        SYSTEM::ScopeLock managerLock(manager.getMutex(), pdMS_TO_TICKS(50));
        if (!managerLock.isLocked()) {
            return;
        }

        const ALARMS::AlarmRule* rules = manager.getRules();
        const uint8_t count = std::min(manager.getCount(), ALARMS::kMaxRules);
        rulesCopy.assign(rules, rules + count);
        ALARMS::AlarmRuntimeState* states = manager.getStates();
        statesCopy.assign(states, states + count);
    }

    struct AlarmRuleStatus {
        char id[ALARMS::kMaxIdLen] = {0};
        bool triggered = false;
        uint32_t lastTriggered = 0;
        float currentValue = NAN;
        bool valid = false;
    };

    std::vector<AlarmRuleStatus, SYSTEM::PsramAllocator<AlarmRuleStatus>> statuses;
    if (!statesCopy.empty()) {
        statuses.reserve(statesCopy.size());
        SensorSnapshot snap = SENSORS::SensorState::getSnapshot();
        auto wifiStats = ctx.wifiSensing ? ctx.wifiSensing->getStats() : WIFISENSING::RssiStats{};

        ALARMS::AlarmInputData inputData;
        inputData.co2 = static_cast<float>(snap.co2);
        inputData.temperature = snap.temp;
        inputData.humidity = snap.humid;
        inputData.wifiVariance = wifiStats.variance;

        const uint32_t nowMs = millis();
        for (size_t i = 0; i < statesCopy.size(); i++) {
            if (!rulesCopy[i].isValid()) {
                continue;
            }

            AlarmRuleStatus item;
            strlcpy(item.id, rulesCopy[i].id, sizeof(item.id));
            item.triggered = statesCopy[i].previouslyTriggered;
            item.lastTriggered = statesCopy[i].lastTriggeredMs;
            if (rulesCopy[i].isBleSource()) {
                item.currentValue = ALARMS::getBleValue(
                    rulesCopy[i].source, rulesCopy[i].bleDeviceMac, nowMs, ctx.bleService);
            } else {
                item.currentValue =
                    ALARMS::AlarmEvaluator::getSensorValue(inputData, rulesCopy[i].source);
            }
            item.valid = true;
            statuses.push_back(item);
        }
    }

    SYSTEM::SpiRamJsonDocument doc(12288);
    doc["type"] = "snapshot";
    doc["channel"] = "alarms";
    JsonObject data = doc["data"].to<JsonObject>();
    data[CONFIG::Keys::kSchemaVersion] = 1;
    JsonArray rulesArr = data[CONFIG::Keys::kRules].to<JsonArray>();

    for (size_t i = 0; i < rulesCopy.size(); i++) {
        const ALARMS::AlarmRule& rule = rulesCopy[i];
        if (!rule.isValid()) {
            continue;
        }

        JsonObject obj = rulesArr.add<JsonObject>();
        obj["id"] = rule.id;
        obj["name"] = rule.name;
        obj["enabled"] = rule.enabled;
        obj["source"] = ALARMS::sourceToString(rule.source);
        obj["operator"] = ALARMS::operatorToString(rule.op);
        obj["threshold"] = static_cast<float>(rule.threshold);
        obj["severity"] = ALARMS::severityToString(rule.severity);
        obj["cooldown_seconds"] = rule.cooldownSeconds;
        obj["created_at"] = static_cast<unsigned long>(rule.createdAt);
        obj["updated_at"] = static_cast<unsigned long>(rule.updatedAt);

        if (rule.isBleSource() && rule.bleDeviceMac[0] != '\0') {
            obj["ble_device_mac"] = rule.bleDeviceMac;
        }

        if (!statuses.empty()) {
            bool found = false;
            bool triggered = false;
            uint32_t lastTriggered = 0;
            float currentValue = NAN;
            for (size_t s = 0; s < statuses.size(); s++) {
                if (statuses[s].valid && strcmp(statuses[s].id, rule.id) == 0) {
                    triggered = statuses[s].triggered;
                    lastTriggered = statuses[s].lastTriggered;
                    currentValue = statuses[s].currentValue;
                    found = true;
                    break;
                }
            }
            if (!found) {
                triggered = false;
                lastTriggered = 0;
            }
            obj["triggered"] = triggered;
            obj["last_triggered"] = static_cast<unsigned long>(lastTriggered);
            if (!std::isnan(currentValue)) {
                obj["current_value"] = currentValue;
            }
        }

        JsonArray channels = obj["notify_channels"].to<JsonArray>();
        if (ALARMS::hasChannel(rule.notifyChannels, ALARMS::NotifyChannel::Telegram)) {
            channels.add("telegram");
        }
        if (ALARMS::hasChannel(rule.notifyChannels, ALARMS::NotifyChannel::Led)) {
            channels.add("led");
        }
        if (ALARMS::hasChannel(rule.notifyChannels, ALARMS::NotifyChannel::Webhook)) {
            channels.add("webhook");
        }
        if (ALARMS::hasChannel(rule.notifyChannels, ALARMS::NotifyChannel::Pushover)) {
            channels.add("pushover");
        }

        if (rule.shellyDeviceCount > 0) {
            JsonArray shellyIds = obj[CONFIG::Keys::kShellyDeviceIds].to<JsonArray>();
            for (uint8_t j = 0; j < rule.shellyDeviceCount && j < ALARMS::kMaxShellyPerRule; j++) {
                if (rule.shellyDeviceIds[j][0] == '\0') {
                    continue;
                }
                shellyIds.add(rule.shellyDeviceIds[j]);
            }
        }
    }

    sendSnapshotDoc(ctx.ws, ctx.fd, doc);
}

}  // namespace API::SYSTEM_WS
