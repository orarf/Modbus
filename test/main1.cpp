#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include <WiFi.h>
#include "freertos/timers.h"
#include <Preferences.h>
#include "TBmanager.h"
#define TB_SERVER "thingsboard.cloud"
#define TB_TOKEN "qvnbqPpZmr5S0w680KPW" 

TBmanager* TBClient = nullptr;  // ประกาศ pointer ไว้
const int LED_PINS[] = {22, 23, 19, 18};
TimerHandle_t timers[4];
bool states[4] = {false, false, false, false};
int freqs[4] = {1, 2, 3, 4};
bool showStatus = false;

// เก็บคำสั่งที่รับมาทาง Serial
String inputBuffer = "";
Preferences preferences;
void handleCommand(String cmd);

void toggleLED(TimerHandle_t xTimer) {
  int id = (int)pvTimerGetTimerID(xTimer);
  states[id] = !states[id];
  digitalWrite(LED_PINS[id], states[id] ? HIGH : LOW);
}

// รีเซ็ต Timer ด้วยความถี่ใหม่
void resetTimer(int id) {
  if (timers[id] != NULL) {
    xTimerStop(timers[id], 0);
    xTimerDelete(timers[id], 0);
  }
  if (freqs[id] <= 0) freqs[id] = 1; // ป้องกัน Hz <= 0
  uint32_t period_ms = (1000 / freqs[id]) / 2;
  timers[id] = xTimerCreate(
    String("LED" + String(id + 1)).c_str(),
    pdMS_TO_TICKS(period_ms),
    pdTRUE,
    (void*)id,
    toggleLED
  );
  xTimerStart(timers[id], 0);
}

// ฟังก์ชันเชื่อมต่อ WiFi (อ่านจาก Preferences)
void connectWiFi() {
  preferences.begin("wifi", true);
  String ssid = preferences.getString("ssid", "");
  String pass = preferences.getString("pass", "");
  preferences.end();

  if (ssid == "") {
    Serial.println("[WiFi] No saved SSID.");
    return;
  }

  Serial.printf("[WiFi] Connecting to SSID: %s\n", ssid.c_str());
  WiFi.begin(ssid.c_str(), pass.c_str());

  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 30) { // รอ 15 วิ
    delay(500);
    Serial.print(".");
    tries++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\n[WiFi] Failed to connect.");
  }
}

// Task อ่าน Serial และจัดการคำสั่ง
void serialTask(void *param) {
  while (1) {
    if (Serial.available()) {
      char ch = Serial.read();
      if (ch == '\n' || ch == '\r') {
        if (inputBuffer.length()) {
          handleCommand(inputBuffer);
          inputBuffer = "";
        }
      } else {
        inputBuffer += ch;
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS); // ลดการใช้ CPU
  }
}

// Task แสดงสถานะ LED ทุก 1 วินาทีถ้าเปิด showStatus
void statusTask(void *param) {
  while (1) {
    if (showStatus) {
      Serial.println("----- LED Status -----");
      for (int i = 0; i < 4; i++) {
        const char* state = digitalRead(LED_PINS[i]) ? "ON " : "OFF";
        const char* timer = (xTimerIsTimerActive(timers[i]) != 0) ? "Running" : "Stopped";
        uint32_t period_ticks = xTimerGetPeriod(timers[i]);
        float period_ms = (float)pdTICKS_TO_MS(period_ticks);
        float freq = 1000.0f / (period_ms * 2.0f);

        Serial.printf("LED%d: %s (Timer: %s, %.2f Hz)\n", i + 1, state, timer, freq);

        // สร้างชื่อ attribute แยกตามหมายเลข LED
        String ledKey = "DEV_LED" + String(i + 1);
        String timerKey = "DEV_TIME" + String(i + 1);
        String freqKey = "DEV_FREQ" + String(i + 1);

        TBClient->sendAttributeData(ledKey.c_str(), state);
        TBClient->sendAttributeData(timerKey.c_str(), timer);
        TBClient->sendAttributeData(freqKey.c_str(), freq);
        TBClient->sendTelemetryData(ledKey.c_str(), state);
        TBClient->sendTelemetryData(timerKey.c_str(), timer);
        TBClient->sendTelemetryData(freqKey.c_str(), freq);
      }
      Serial.println("----------------------");
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

// ฟังก์ชันจัดการคำสั่งที่รับมาจาก Serial
void handleCommand(String cmd) {
  cmd.trim();
  // คำสั่งเปลี่ยน WiFi แบบ "WIFI <ssid> <password>"
  if (cmd.startsWith("WIFI ")) {
    int firstSpace = cmd.indexOf(' ');
    int secondSpace = cmd.indexOf(' ', firstSpace + 1);
    if (secondSpace > firstSpace) {
      String newSSID = cmd.substring(firstSpace + 1, secondSpace);
      String newPass = cmd.substring(secondSpace + 1);

      if (newSSID.length() > 0 && newPass.length() > 0) {
        preferences.begin("wifi", false);
        preferences.putString("ssid", newSSID);
        preferences.putString("pass", newPass);
        preferences.end();

        Serial.printf("[WiFi] Saved new credentials: SSID=%s\n", newSSID.c_str());
        Serial.println("Restart ESP to connect new WiFi.");
      } else {
        Serial.println("Invalid WIFI command. Use: WIFI <ssid> <password>");
      }
    } else {
      Serial.println("Invalid WIFI command. Use: WIFI <ssid> <password>");
    }
    return;
  }
  cmd.toUpperCase();

  // คำสั่งควบคุม LED แบบเดิม
  if (cmd == "ON ALL") {
    for (int i = 0; i < 4; i++) {
      xTimerStart(timers[i], 0);
      states[i] = false; // toggle ใหม่
      preferences.begin("ledprefs", false);
      preferences.putBool(("state" + String(i)).c_str(), true);
      preferences.end();
    }
    Serial.println("All LEDs ON (Timers started).");
  } else if (cmd == "OFF ALL") {
    for (int i = 0; i < 4; i++) {
      xTimerStop(timers[i], 0);
      states[i] = false;
      digitalWrite(LED_PINS[i], LOW);
      preferences.begin("ledprefs", false);
      preferences.putBool(("state" + String(i)).c_str(), false);
      preferences.end();
    }
    Serial.println("All LEDs OFF (Timers stopped).");
  } else if (cmd == "STATUS ON") {
    showStatus = true;
    Serial.println("Status output ENABLED.");
  } else if (cmd == "STATUS OFF") {
    showStatus = false;
    Serial.println("Status output DISABLED.");
  } else if (cmd == "STATUS") {
    Serial.println("----- LED Status -----");
    for (int i = 0; i < 4; i++) {
      Serial.printf("LED%d: %s | Frequency: %d Hz\n", i + 1, states[i] ? "ON" : "OFF", freqs[i]);
    }
    Serial.println("----------------------");
  } else if (cmd.startsWith("ON ")) {
    int id = cmd.substring(3).toInt() - 1;
    if (id >= 0 && id < 4) {
      xTimerStart(timers[id], 0);
      states[id] = false;  // toggle เริ่มนับใหม่
      preferences.begin("ledprefs", false);
      preferences.putBool(("state" + String(id)).c_str(), true);
      preferences.end();
      Serial.printf("LED%d ON (Timer started)\n", id + 1);
    } else {
      Serial.println("Invalid LED number.");
    }
  } else if (cmd.startsWith("OFF ")) {
    int id = cmd.substring(4).toInt() - 1;
    if (id >= 0 && id < 4) {
      xTimerStop(timers[id], 0);
      states[id] = false;
      digitalWrite(LED_PINS[id], LOW);
      preferences.begin("ledprefs", false);
      preferences.putBool(("state" + String(id)).c_str(), false);
      preferences.end();
      Serial.printf("LED%d OFF (Timer stopped)\n", id + 1);
    } else {
      Serial.println("Invalid LED number.");
    }
  } else if (cmd.startsWith("FREQ ")) {
    int firstSpace = cmd.indexOf(' ');
    int secondSpace = cmd.indexOf(' ', firstSpace + 1);
    if (firstSpace > 0 && secondSpace > firstSpace) {
      int ledNum = cmd.substring(firstSpace + 1, secondSpace).toInt() - 1;
      int newFreq = cmd.substring(secondSpace + 1).toInt();
      if (ledNum >= 0 && ledNum < 4 && newFreq > 0) {
        freqs[ledNum] = newFreq;
        preferences.begin("ledprefs", false);
        String key = "freq" + String(ledNum);
        preferences.putInt(key.c_str(), newFreq);
        preferences.end();
        resetTimer(ledNum);
        Serial.printf("LED%d frequency set to %d Hz\n", ledNum + 1, newFreq);
      } else {
        Serial.println("Invalid LED number or frequency.");
      }
    } else {
      Serial.println("Invalid FREQ command format.");
      Serial.println("Use: FREQ <led#> <hz>");
    }
  } else {
    Serial.println("❌ Unknown command.");
    Serial.println("Use: ON <1-4>, OFF <1-4>, ON ALL, OFF ALL, STATUS, STATUS ON, STATUS OFF, FREQ <led#> <hz>, WIFI <ssid> <password>");
  }
}

// ตัวอย่างฟังก์ชันรับ RPC (ThingBoard)
void RPC_TEST_process1(const JsonVariantConst &value, JsonDocument &response) {
  Serial.println("=== รับ RPC จาก ThingBoard ===");
  serializeJsonPretty(value, Serial);
  Serial.println();
  StaticJsonDocument<JSON_OBJECT_SIZE(4)> innerDoc;
  innerDoc["string"] = "exampleResponseString";
  innerDoc["int"] = 5;
  innerDoc["float"] = 5.0f;
  innerDoc["bool"] = true;
  response["json_data"] = innerDoc;
}

void RPC_TEST_process2(const JsonVariantConst &value, JsonDocument &response) {
  Serial.println("=== รับ RPC จาก ThingBoard ===");
  serializeJsonPretty(value, Serial);
  Serial.println();
  StaticJsonDocument<JSON_OBJECT_SIZE(4)> innerDoc;
  innerDoc["string"] = "exampleResponseString";
  innerDoc["int"] = 5;
  innerDoc["float"] = 5.0f;
  innerDoc["bool"] = true;
  response["json_data"] = innerDoc;
}

void RPC_TEST_process3(const JsonVariantConst &value, JsonDocument &response) {
  Serial.println("=== รับ RPC จาก ThingBoard ===");
  serializeJsonPretty(value, Serial);
  Serial.println();
  StaticJsonDocument<JSON_OBJECT_SIZE(4)> innerDoc;
  innerDoc["string"] = "exampleResponseString";
  innerDoc["int"] = 5;
  innerDoc["float"] = 5.0f;
  innerDoc["bool"] = true;
  response["json_data"] = innerDoc;
}

// กำหนด callback RPC
const RPC_Callback fncallbacks[MAX_RPC_SUBSCRIPTIONS] = {
  { "USER_CONTROL_01", RPC_TEST_process1 },
  { "USER_CONTROL_02", RPC_TEST_process2 },
  { "USER_CONTROL_03", RPC_TEST_process3 },
};

void setup() {
  Serial.begin(9600);

  // โหลดค่า WiFi จาก Preferences ก่อนสร้าง TBClient
  preferences.begin("wifi", true);
  String ssid = preferences.getString("ssid", "");
  String pass = preferences.getString("pass", "");
  preferences.end();

  if (ssid == "" || pass == "") {
    Serial.println("No saved WiFi credentials found!");
  } else {
    Serial.printf("Loaded WiFi credentials SSID=%s\n", ssid.c_str());
  }

  // สร้าง TBmanager ด้วยค่า ssid, pass ที่อ่านมา
  TBClient = new TBmanager(ssid.c_str(), pass.c_str(), TB_SERVER, TB_TOKEN);

  TBClient->RPCRoute(fncallbacks);

  connectWiFi();
  TBClient->begin();

  delay(5000);
  Serial.println(TBClient->get_ipAddr());
  Serial.println(TBClient->get_SSID());

  // โหลดสถานะและความถี่จาก Preferences
  preferences.begin("ledprefs", true);
  for (int i = 0; i < 4; i++) {
    String keyFreq = "freq" + String(i);
    freqs[i] = preferences.getInt(keyFreq.c_str(), freqs[i]);
    String keyState = "state" + String(i);
    states[i] = preferences.getBool(keyState.c_str(), false);
  }
  preferences.end();

  for (int i = 0; i < 4; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    // สร้าง timer ด้วยความถี่ที่โหลดมา
    uint32_t period_ms = (1000 / freqs[i]) / 2;
    timers[i] = xTimerCreate(
      String("LED" + String(i + 1)).c_str(),
      pdMS_TO_TICKS(period_ms),
      pdTRUE,
      (void*)i,
      toggleLED
    );
    if (states[i]) {
      xTimerStart(timers[i], 0);
    } else {
      digitalWrite(LED_PINS[i], LOW);
    }
  }

  // สร้าง FreeRTOS task แยกอ่าน Serial และแสดงสถานะ
  xTaskCreate(serialTask, "SerialTask", 4096, NULL, 1, NULL);
  xTaskCreate(statusTask, "StatusTask", 2048, NULL, 1, NULL);

  Serial.println("Ready for commands:");
  Serial.println("ON <1-4>, OFF <1-4>, ON ALL, OFF ALL, STATUS, STATUS ON, STATUS OFF, FREQ <led#> <hz>, WIFI <ssid> <password>");
}

void loop() {
  // if (TBClient) {
  //   TBClient->sendAttributeData("DEV_KEY", random());
  //   TBClient->sendAttributeData("DEV_STR", "Send String IP xx,xx,xx,xxx");
  //   TBClient->sendAttributeData("DEV_IP", TBClient->get_ipAddr().c_str());
  //   TBClient->sendTelemetryData("DEV_STR", "Send String IP xx,xx,xx,xxx");
  //   TBClient->sendTelemetryData("DEV_IP", TBClient->get_ipAddr().c_str());
  // }
  // delay(500);
}