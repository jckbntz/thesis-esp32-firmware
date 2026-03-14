#include "wifi_manager.h"
#include <Arduino.h>

namespace WiFiManager
{

    void connectWiFi()
    {
        Serial.print("\n\nConnecting to WiFi");
        WiFi.begin(WIFI_SSID, WIFI_PASS);

        while (WiFi.status() != WL_CONNECTED)
        {
            delay(500);
            Serial.print(".");
        }

        Serial.printf("\n[WiFi] connected — IP: %s\n", WiFi.localIP().toString().c_str());
    }
}
