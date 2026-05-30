/**
 * @file BleDataProvider.h
 * @brief Utility functions for fetching BLE sensor data for alarm evaluation
 * 
 * Provides a unified interface for getting BLE thermometer readings,
 * used by AlarmCoordinator and API handlers.
 */

#pragma once

#include "../types/AlarmEnums.h"
#include "../types/AlarmConstants.h"
#include "../../ble/BleService.h"
#include <cmath>

namespace ALARMS {

/**
 * Get BLE sensor value for a specific source and device MAC.
 * 
 * @param source AlarmSource (must be BleTemperature or BleHumidity)
 * @param bleMac Device MAC address (e.g., "A4:C1:38:XX:XX:XX")
 * @param nowMs Current time in milliseconds (from millis())
 * @param staleTimeoutMs Maximum age of reading before considered stale
 * @return Sensor value or NAN if unavailable/stale/error
 */
inline float getBleValue(
    AlarmSource source, 
    const char* bleMac, 
    uint32_t nowMs,
    BLE::BleService* bleService,
    uint32_t staleTimeoutMs = kBleStaleTimeoutMs
) {
    // Validate source is BLE type
    if (source != AlarmSource::BleTemperature && source != AlarmSource::BleHumidity) {
        return NAN;
    }
    
    // Validate MAC
    if (!bleMac || bleMac[0] == '\0') {
        return NAN;
    }
    
    // Check if BLE is running
    if (!bleService || !bleService->isRunning()) {
        return NAN;
    }
    
    // Get cached data
    float temp, humid;
    uint8_t batt;
    int8_t rssi;
    uint32_t lastSeen;
    
    if (!bleService->getCachedDeviceData(bleMac, temp, humid, batt, rssi, lastSeen)) {
        return NAN;
    }
    
    // Check if data is stale (handle millis() overflow)
    uint32_t age = (nowMs >= lastSeen) 
        ? (nowMs - lastSeen) 
        : (0xFFFFFFFF - lastSeen + nowMs + 1);
        
    if (age > staleTimeoutMs) {
        return NAN;
    }
    
    return (source == AlarmSource::BleTemperature) ? temp : humid;
}

/**
 * Populate BLE sensor values in AlarmInputData for a specific rule.
 * Sets bleTemp and bleHumid fields based on the rule's source and bleDeviceMac.
 * 
 * @param isBleSource Whether the rule uses a BLE source
 * @param source The alarm source (only relevant if isBleSource is true)
 * @param bleMac BLE device MAC address
 * @param nowMs Current time in milliseconds
 * @param bleService Pointer to BleService instance
 * @param outBleTemp Output: temperature value (set to NAN if not applicable)
 * @param outBleHumid Output: humidity value (set to NAN if not applicable)
 */
inline void populateBleValues(
    bool isBleSource,
    AlarmSource source,
    const char* bleMac,
    uint32_t nowMs,
    BLE::BleService* bleService,
    float& outBleTemp,
    float& outBleHumid
) {
    if (!isBleSource || !bleMac || bleMac[0] == '\0') {
        outBleTemp = NAN;
        outBleHumid = NAN;
        return;
    }
    
    if (!bleService || !bleService->isRunning()) {
        outBleTemp = NAN;
        outBleHumid = NAN;
        return;
    }
    
    float temp, humid;
    uint8_t batt;
    int8_t rssi;
    uint32_t lastSeen;
    
    if (!bleService->getCachedDeviceData(bleMac, temp, humid, batt, rssi, lastSeen)) {
        outBleTemp = NAN;
        outBleHumid = NAN;
        return;
    }
    
    // Check if data is stale (handle millis() overflow)
    uint32_t age = (nowMs >= lastSeen) 
        ? (nowMs - lastSeen) 
        : (0xFFFFFFFF - lastSeen + nowMs + 1);
        
    if (age > kBleStaleTimeoutMs) {
        outBleTemp = NAN;
        outBleHumid = NAN;
        return;
    }
    
    outBleTemp = temp;
    outBleHumid = humid;
}

}  // namespace ALARMS
