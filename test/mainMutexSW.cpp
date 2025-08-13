#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <WiFi.h>
#include <TBmanager.h>
#include <Preferences.h>

Preferences prefs;
#define USER_WIFI_SSID "Pooh"
#define USER_WIFI_PASSWORD  "87654321"
#define TB_SERVER "thingsboard.cloud"
#define TB_TOKEN "qvnbqPpZmr5S0w680KPW"  // Device TOKEN ID

TBmanager tbClient(USER_WIFI_SSID, USER_WIFI_PASSWORD,TB_SERVER,TB_TOKEN);

// ขา I/O
const int LED_PIN[] = {27, 23, 19, 18};
String inputString = ""; 

SemaphoreHandle_t xMutex;  // สร้าง Mutex
bool ledShouldBeOn1 = false;  // ตัวแปรที่ใช้ร่วมกัน
bool ledShouldBeOn2 = false;
bool ledShouldBeOn3 = false;
bool ledShouldBeOn4 = false;

void TaskButton(void *pvParameters) {
  while (1) {    
      if (xSemaphoreTake(xMutex, portMAX_DELAY)) {
      if (Serial.available()) {
       inputString = Serial.readStringUntil('\n');  // อ่านค่าจนกว่าจะเจอ Enter
       inputString.trim(); // ลบช่องว่างหรือ \r\n ด้านหน้า-หลัง

       if (inputString == "on1") {
      ledShouldBeOn1 = true; // เปิดไฟ
      Serial.println("LED ติดแล้ว");
    } else if (inputString == "off1") {
      ledShouldBeOn1 = false;  // ดับไฟ
      Serial.println("LED ดับแล้ว");
    } else if (inputString == "on2") {
      ledShouldBeOn2 = true;  // ดับไฟ
      Serial.println("LED ดับแล้ว");
    } else if (inputString == "off2") {
      ledShouldBeOn2 = false;  // ดับไฟ
      Serial.println("LED ดับแล้ว");
    } else if (inputString == "on3") {
      ledShouldBeOn3 = true;  // ดับไฟ
      Serial.println("LED ดับแล้ว");
    } else if (inputString == "off3") {
      ledShouldBeOn3 = false;  // ดับไฟ
      Serial.println("LED ดับแล้ว");
    } else if (inputString == "on4") {
      ledShouldBeOn4 = true;  // ดับไฟ
      Serial.println("LED ดับแล้ว");
    } else if (inputString == "off4") {
      ledShouldBeOn4 = false;  // ดับไฟ
      Serial.println("LED ดับแล้ว");
    } else {
      Serial.println("พิมพ์แค่ 'on' หรือ 'off' เท่านั้นนะ");
    }
  }
        xSemaphoreGive(xMutex);
      }
      
    vTaskDelay(50 / portTICK_PERIOD_MS);  // อ่านทุก 50ms
    }    
  }




void RPC_TEST_process1(const JsonVariantConst &data, JsonDocument &response) {
  Serial.println("Received the json RPC method RPC_TEST_process1");
   if (xSemaphoreTake(xMutex, portMAX_DELAY)) {
     if (data == "on1") {
      ledShouldBeOn1 = true; // เปิดไฟ
    } else if (data == "off1") {
      ledShouldBeOn1 = false;  // ดับไฟ
    } else if (data == "on2") {
      ledShouldBeOn2 = true;  // ดับไฟ
    } else if (data == "off2") {
      ledShouldBeOn2 = false;  // ดับไฟ
    } else if (data == "on3") {
      ledShouldBeOn3 = true;  // ดับไฟ
    } else if (data == "off3") {
      ledShouldBeOn3 = false;  // ดับไฟ
    } else if (data == "on4") {
      ledShouldBeOn4 = true;  // ดับไฟ
    } else if (data == "off4") {
      ledShouldBeOn4 = false;  // ดับไฟ
    }
     xSemaphoreGive(xMutex);
    }
  // Size of the response document needs to be configured to the size of the innerDoc + 1.
  StaticJsonDocument<JSON_OBJECT_SIZE(4)> innerDoc;
  innerDoc["string"] = "exampleResponseString";
  innerDoc["int"] = 5;
  innerDoc["float"] = 5.0f;
  innerDoc["bool"] = true;
  response["json_data"] = innerDoc;
}

void RPC_TEST_process2(const JsonVariantConst &data, JsonDocument &response) {
  Serial.println("Received the json RPC method RPC_TEST_process2");

  // Size of the response document needs to be configured to the size of the innerDoc + 1.
  StaticJsonDocument<JSON_OBJECT_SIZE(4)> innerDoc;
  innerDoc["string"] = "exampleResponseString RPC_TEST_process2";
  innerDoc["int"] = 5;
  innerDoc["float"] = 5.0f;
  innerDoc["bool"] = true;
  response["json_data"] = innerDoc;
}

void RPC_TEST_process3(const JsonVariantConst &data, JsonDocument &response) {
  Serial.println("Received the json RPC method RPC_TEST_process3 ");

  // Size of the response document needs to be configured to the size of the innerDoc + 1.
  StaticJsonDocument<JSON_OBJECT_SIZE(4)> innerDoc;
  innerDoc["string"] = "exampleResponseString";
  innerDoc["int"] = 5;
  innerDoc["float"] = 5.0f;
  innerDoc["bool"] = true;
  response["json_data"] = innerDoc;
}

    const RPC_Callback xcallbacks[3] = {
      // Requires additional memory in the JsonDocument for the JsonDocument that will be copied into the response
      { "USER_CONTROL_01",    RPC_TEST_process1 },
      // Requires additional memory in the JsonDocument for 5 key-value pairs that do not copy their value into the JsonDocument itself
      { "USER_CONTROL_02",    RPC_TEST_process2 },
      // Internal size can be 0, because if we use the JsonDocument as a JsonVariant and then set the value we do not require additional memory
      { "USER_CONTROL_03",     RPC_TEST_process3 }
    };

void TaskLED(void *pvParameters) {
  bool previousState1 = !ledShouldBeOn1;
  bool previousState2 = !ledShouldBeOn2;
  bool previousState3 = !ledShouldBeOn3;
  bool previousState4 = !ledShouldBeOn4;

  prefs.begin("led_states", false);  // เปิด preferences (read-write)

  while (1) {
    bool currentState1;
    bool currentState2;
    bool currentState3;
    bool currentState4;

    if (xSemaphoreTake(xMutex, portMAX_DELAY)) {
      currentState1 = ledShouldBeOn1;
      currentState2 = ledShouldBeOn2;
      currentState3 = ledShouldBeOn3;
      currentState4 = ledShouldBeOn4;
      xSemaphoreGive(xMutex);
    }

    // เขียนค่าออกไปยัง GPIO
    digitalWrite(LED_PIN[0], currentState1 ? HIGH : LOW);
    digitalWrite(LED_PIN[1], currentState2 ? HIGH : LOW);
    digitalWrite(LED_PIN[2], currentState3 ? HIGH : LOW);
    digitalWrite(LED_PIN[3], currentState4 ? HIGH : LOW);

    // บันทึกค่าเฉพาะเมื่อมีการเปลี่ยนแปลง
    if (currentState1 != previousState1) {
      prefs.putBool("led1", currentState1);
      previousState1 = currentState1;
    }
    if (currentState2 != previousState2) {
      prefs.putBool("led2", currentState2);
      previousState2 = currentState2;
    }
    if (currentState3 != previousState3) {
      prefs.putBool("led3", currentState3);
      previousState3 = currentState3;
    }
    if (currentState4 != previousState4) {
      prefs.putBool("led4", currentState4);
      previousState4 = currentState4;
    }

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }

  prefs.end();  // (จริง ๆ ไม่จำเป็นใน loop ไม่จบ แต่ใส่ไว้เผื่อ copy ไปใช้ที่อื่น)
}

void setup() {
  Serial.begin(9600);
  prefs.begin("led_states", true);  // true = read-only

  ledShouldBeOn1 = prefs.getBool("led1", false);
  ledShouldBeOn2 = prefs.getBool("led2", false);
  ledShouldBeOn3 = prefs.getBool("led3", false);
  ledShouldBeOn4 = prefs.getBool("led4", false);

  prefs.end();
  tbClient.RPCRoute(xcallbacks);
  tbClient.begin();
  
  for (int i = 0; i < 4; i++) {
    pinMode(LED_PIN[i], OUTPUT);
  }
  // สร้าง Mutex
  xMutex = xSemaphoreCreateMutex();

  if (xMutex == NULL) {
    Serial.println("❌ สร้าง Mutex ไม่สำเร็จ");
    while (1);
  } else {
    Serial.println("✅ สร้าง Mutex สำเร็จ");
  }
  
  // สร้าง Task
  xTaskCreate(TaskButton, "Button Task", 2048, NULL, 1, NULL);
  xTaskCreate(TaskLED, "LED Task", 2048, NULL, 1, NULL);
  Serial.println("พิมพ์ 'on' หรือ 'off' เพื่อสั่งไฟ");
}

void loop() {
  
}
