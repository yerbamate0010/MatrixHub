#pragma once

#include "../../../alarms/types/AlarmTypes.h"
#include "../../../system/utils/json/JsonResponseWriter.h"

namespace API {

struct RuleStatus {
    char id[ALARMS::kMaxIdLen] = {0};
    bool triggered = false;
    uint32_t lastTriggered = 0;
    float currentValue = NAN;
    bool valid = false;
};

class AlarmRulesSerializer {
public:
    /**
     * @brief Serialize alarm rules list to JSON
     * 
     * @param w JSON writer
     * @param rules Array of rules
     * @param count Number of rules
     * @param statuses Optional status array (can be null)
     * @param statusCount Number of statuses
     * @param includeStatus Whether to include runtime status fields
     * @return true on success
     */
    static bool serialize(Utils::JsonResponseWriter& w, 
                         const ALARMS::AlarmRule* rules, 
                         uint8_t count,
                         const RuleStatus* statuses,
                         uint8_t statusCount,
                         bool includeStatus);
};

} // namespace API
