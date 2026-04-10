#pragma once
#include <WiFi.h>
#include "node_selector.h"

namespace WiFiManager
{
    void connectWiFi();
    void maintainConnection();
}