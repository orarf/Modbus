#ifndef TBMANAGER_H
#define TBMANAGER_H

#include <WiFi.h>
#include <ThingsBoard.h>
#include <ArduinoJson.h>

#define MAX_RPC_SUBSCRIPTIONS 10  // เปลี่ยนค่าตามจำนวน RPC callback ที่ต้องการรองรับ

// โครงสร้างสำหรับ Callback
typedef void (*RPC_CallbackFn)(const JsonVariantConst &data, JsonDocument &response);
struct RPC_Callback {
  const char *methodName;
  RPC_CallbackFn callback;
};

class TBmanager {
  public:
    TBmanager(const char* ssid, const char* password, const char* server, const char* token)
      : wifi_ssid(ssid), wifi_password(password), tb_server(server), tb_token(token),
        client(), tb(client) {}

    void begin() {
      WiFi.begin(wifi_ssid, wifi_password);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      Serial.println("\nWiFi connected!");
      client.setInsecure(); // สำหรับ SSL แบบไม่ตรวจสอบ cert
      reconnect();
    }

    void loop() {
      if (!tb.connected()) {
        reconnect();
      }
      tb.loop();
    }

    void sendTelemetryData(const char* key, float value) {
      if (tb.connected()) {
        tb.sendTelemetryFloat(key, value);
      }
    }

    void sendAttributeData(const char* key, bool value) {
      if (tb.connected()) {
        tb.sendAttributeBool(key, value);
      }
    }

    String get_ipAddr() {
      return WiFi.localIP().toString();
    }

    String get_SSID() {
      return WiFi.SSID();
    }

    void RPCRoute(const RPC_Callback *callbacks, size_t count) {
      rpcCount = (count > MAX_RPC_SUBSCRIPTIONS) ? MAX_RPC_SUBSCRIPTIONS : count;
      for (size_t i = 0; i < rpcCount; ++i) {
        rpcCallbacks[i] = callbacks[i];
        tb.RPC_Subscribe(rpcCallbacks[i].methodName, rpcCallbacks[i].callback);
      }
    }

  private:
    const char* wifi_ssid;
    const char* wifi_password;
    const char* tb_server;
    const char* tb_token;

    WiFiClientSecure client;
    ThingsBoard tb;

    RPC_Callback rpcCallbacks[MAX_RPC_SUBSCRIPTIONS];
    size_t rpcCount = 0;

    void reconnect() {
      while (!tb.connected()) {
        Serial.print("Connecting to ThingsBoard ... ");
        if (tb.connect(tb_server, tb_token)) {
          Serial.println("[DONE]");
          // Subscribe again after reconnect
          for (size_t i = 0; i < rpcCount; ++i) {
            tb.RPC_Subscribe(rpcCallbacks[i].methodName, rpcCallbacks[i].callback);
          }
        } else {
          Serial.println("[FAILED]");
          delay(1000);
        }
      }
    }
};

#endif // TBMANAGER_H
