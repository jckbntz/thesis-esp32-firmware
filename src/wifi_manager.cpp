#include "wifi_manager.h"
#include <Arduino.h>

namespace WiFiManager
{
    void connectWiFi()
    {
        Serial.print("\n[WiFi] Connecting");
        WiFi.begin(WIFI_SSID, WIFI_PASS);

        while (WiFi.status() != WL_CONNECTED)
        {
            delay(500);
            Serial.print(".");
        }

        Serial.printf("\n[WiFi] Connected — IP: %s\n",
                      WiFi.localIP().toString().c_str());
    }
}