#!/bin/bash
# Air Mouse API Helper Script
# Usage: source this file, then use the functions

ESP_IP="${ESP_IP:-192.168.0.22}"
ESP_USER="${ESP_USER:-admin}"
ESP_PASS="${ESP_PASS:-admin}"

# Get JWT token
get_token() {
    curl -s -X POST -H "Content-Type: application/json" \
        -d "{\"username\":\"$ESP_USER\",\"password\":\"$ESP_PASS\"}" \
        "http://$ESP_IP/rest/signIn" | jq -r '.access_token'
}

# Air Mouse status
airmouse_status() {
    local token=$(get_token)
    curl -s -H "Authorization: Bearer $token" \
        "http://$ESP_IP/api/airmouse/status" | jq .
}

# Enable Air Mouse with default settings
airmouse_enable() {
    local token=$(get_token)
    curl -s -X POST -H "Authorization: Bearer $token" \
        -H "Content-Type: application/json" \
        -d '{"enabled":true,"sensitivity_x":8.0,"sensitivity_y":8.0,"deadzone":0.5,"shake_threshold_g":2.5,"click_debounce_ms":400}' \
        "http://$ESP_IP/api/airmouse/config" | jq .
}

# Disable Air Mouse
airmouse_disable() {
    local token=$(get_token)
    curl -s -X POST -H "Authorization: Bearer $token" \
        -H "Content-Type: application/json" \
        -d '{"enabled":false}' \
        "http://$ESP_IP/api/airmouse/config" | jq .
}

# Calibrate gyroscope
airmouse_calibrate() {
    local token=$(get_token)
    curl -s -X POST -H "Authorization: Bearer $token" \
        "http://$ESP_IP/api/airmouse/calibrate" | jq .
}

# Update sensitivity (x, y)
airmouse_sensitivity() {
    local sens_x="${1:-8.0}"
    local sens_y="${2:-8.0}"
    local token=$(get_token)
    curl -s -X POST -H "Authorization: Bearer $token" \
        -H "Content-Type: application/json" \
        -d "{\"sensitivity_x\":$sens_x,\"sensitivity_y\":$sens_y}" \
        "http://$ESP_IP/api/airmouse/config" | jq .
}

echo "Air Mouse API helper loaded. Commands:"
echo "  airmouse_status     - Get current status"
echo "  airmouse_enable     - Enable with default settings"
echo "  airmouse_disable    - Disable"
echo "  airmouse_calibrate  - Trigger gyro calibration"
echo "  airmouse_sensitivity X Y - Set sensitivity (1-50)"
echo ""
echo "Usage: ESP_IP=192.168.0.22 source scripts/airmouse_api.sh"
