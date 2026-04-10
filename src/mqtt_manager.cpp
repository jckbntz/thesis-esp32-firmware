#include "mqtt_manager.h"
#include "node_selector.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>

// ─────────────────────────────────────────────────────────────────────────────
//  Internal state
// ─────────────────────────────────────────────────────────────────────────────
namespace
{
    static WiFiClient    wifiClient;
    static PubSubClient  mqtt(wifiClient);

    // ── Reconnect helper ──────────────────────────────────────────────────────
    //  Called automatically by loop() and publishAll() if the broker drops.
    //  Non-blocking retry: tries once, returns success/fail immediately.
    static bool reconnect()
    {
        if (mqtt.connected()) return true;

        Serial.printf("[MQTT] Connecting to %s:%d … ", MQTT_BROKER, MQTT_PORT);

        bool ok;
        if (strlen(MQTT_USER) > 0)
            ok = mqtt.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS);
        else
            ok = mqtt.connect(MQTT_CLIENT_ID);

        if (ok)
            Serial.println("connected.");
        else
            Serial.printf("failed (rc=%d) — will retry next tick.\n",
                          mqtt.state());

        return ok;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
namespace MqttManager
{
    void begin()
    {
        mqtt.setServer(MQTT_BROKER, MQTT_PORT);
        // PubSubClient default keep-alive is 15 s — fine for this use case.
        // Increase SEND_INTERVAL_MS beyond 15 s and bump setKeepAlive() to match.

        // First connection attempt — block until successful so setup() is clean.
        Serial.println("[MQTT] Establishing initial connection…");
        while (!reconnect()) delay(2000);
    }

    void loop()
    {
        if (!mqtt.connected()) reconnect();
        mqtt.loop();   // processes keep-alive PINGREQ/PINGRESP
    }

    // ── publishAll() ──────────────────────────────────────────────────────────
    //  Publishes each sensor's currentFlow (L/min) as a plain decimal string.
    //
    //  Topic:   daloyko/<USER_ID>/<sensor.topic>
    //  Payload: "3.47"   (two decimal places, no unit suffix)
    //
    //  Node-RED splits on "/" and uses the third segment as the key into its
    //  cloudMemory map, so the topic format here must match exactly.
    bool publishAll(const FlowSensor* sensors, int count)
    {
        if (!mqtt.connected() && !reconnect()) return false;

        bool allOk = true;
        char topic[128];
        char payload[16];

        for (int i = 0; i < count; i++)
        {
            // Build topic: daloyko/<USER_ID>/<sensorTopic>
            snprintf(topic, sizeof(topic),
                     "daloyko/%s/%s",
                     USER_ID,
                     sensors[i].topic);

            // Payload: plain float string, 2 dp
            snprintf(payload, sizeof(payload),
                     "%.2f",
                     sensors[i].currentFlow);

            const bool ok = mqtt.publish(topic, payload, /*retain=*/false);
            if (!ok)
            {
                Serial.printf("[MQTT] ✗ Publish failed: %s\n", topic);
                allOk = false;
            }
        }

        return allOk;
    }

    // ── publishHeartbeat() ────────────────────────────────────────────────────
    //  Topic:   daloyko/<USER_ID>/<HEARTBEAT_TOPIC>
    //  Payload: {"uptime":<seconds since boot>, "rssi":<WiFi signal dBm>}
    //
    //  Node-RED only needs to see the topic arrive — it stamps Date.now() into
    //  Firebase and ignores the payload entirely.  We send diagnostics anyway
    //  (Level 2) because it's free and the React dashboard can use them later.
    //
    //  Called on its own 10-second timer in main.cpp loop(), completely
    //  independent from the sensor publish interval.  millis() never blocks.
    bool publishHeartbeat()
    {
        if (!mqtt.connected() && !reconnect()) return false;

        char topic[128];
        snprintf(topic, sizeof(topic),
                 "daloyko/%s/%s", USER_ID, HEARTBEAT_TOPIC);

        // Build JSON: {"uptime":12345, "rssi":-66}
        char payload[64];
        snprintf(payload, sizeof(payload),
                 "{\"uptime\":%lu,\"rssi\":%d}",
                 millis() / 1000UL,
                 WiFi.RSSI());

        const bool ok = mqtt.publish(topic, payload, /*retain=*/false);
        if (ok)
            Serial.printf("[MQTT] ♥ Heartbeat → %s  %s\n", topic, payload);
        else
            Serial.printf("[MQTT] ✗ Heartbeat failed: %s\n", topic);

        return ok;
    }

    // ── printAll() ────────────────────────────────────────────────────────────
    //  Human-readable Serial dump — identical content to what gets published,
    //  formatted for easy reading.  Zero MQTT cost.
    void printAll(const FlowSensor* sensors, int count)
    {
        Serial.printf("\n[%lu ms] Publishing %d sensor(s):\n", millis(), count);
        Serial.println("  Topic                                       Flow      Total");
        Serial.println("  ─────────────────────────────────────────── ───────── ──────────");
        for (int i = 0; i < count; i++)
        {
            char topic[128];
            snprintf(topic, sizeof(topic),
                     "daloyko/%s/%s", USER_ID, sensors[i].topic);

            Serial.printf("  %-43s %6.2f lpm  %7.3f L\n",
                          topic,
                          sensors[i].currentFlow,
                          sensors[i].totalLitres);
        }
        Serial.println();
    }
}