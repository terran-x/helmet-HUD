#include <Arduino.h>
#include <WiFi.h>
#include "Network.h"

namespace {
constexpr char AP_SSID[] = "HelmetHUD";
constexpr char AP_PASSWORD[] = "hud12345"; // must be 8+ characters
constexpr uint8_t AP_CHANNEL = 1;
}

uint8_t connectedDeviceCount() {
    return WiFi.softAPgetStationNum();
}

bool startAccessPoint() {
    WiFi.mode(WIFI_AP);

    if (!WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL)) {
        Serial.println("SoftAP start failed");
        return false;
    }

    Serial.print("SoftAP IP: ");
    Serial.println(WiFi.softAPIP());
    return true;
}