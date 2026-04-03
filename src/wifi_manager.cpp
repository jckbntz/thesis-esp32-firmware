#include "wifi_manager.h"

namespace WiFiManager
{
    void connectWiFi()
    {
        Serial.printf("\n[WiFi] Connecting to \"%s\"", WIFI_SSID);
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASS);

        while (WiFi.status() != WL_CONNECTED)
        {
            delay(500);
            Serial.print(".");
        }

        Serial.printf("\n[WiFi] Connected — IP: %s  RSSI: %d dBm\n",
                      WiFi.localIP().toString().c_str(),
                      WiFi.RSSI());
    }

    void maintainConnection()
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.println("[WiFi] Connection lost — reconnecting…");
            WiFi.disconnect();
            connectWiFi();
        }
    }
}
