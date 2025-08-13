#include <Arduino.h>
#include <Preferences.h>
#include "TBmanager.h"
#include "commandline.h" 

// Global objects
TBmanager* tbManager;
CommandLineManager* cliManager;
Preferences preferences;
void RPC_TEST_process1(const JsonVariantConst &data, JsonDocument &response)
{
  Serial.println("Received the json RPC method RPC_TEST_process1");
  StaticJsonDocument<JSON_OBJECT_SIZE(4)> innerDoc;
  innerDoc["string"] = "exampleResponseString";
  innerDoc["int"] = 5;
  innerDoc["float"] = 5.0f;
  innerDoc["bool"] = true;
  response["json_data"] = innerDoc;
}

void RPC_TEST_process2(const JsonVariantConst &data, JsonDocument &response)
{
  Serial.println("Received the json RPC method RPC_TEST_process2");
  StaticJsonDocument<JSON_OBJECT_SIZE(4)> innerDoc;
  innerDoc["string"] = "exampleResponseString RPC_TEST_process2";
  innerDoc["int"] = 5;
  innerDoc["float"] = 5.0f;
  innerDoc["bool"] = true;
  response["json_data"] = innerDoc;
}

void RPC_TEST_process3(const JsonVariantConst &data, JsonDocument &response)
{
  Serial.println("Received the json RPC method RPC_TEST_process3 ");
  StaticJsonDocument<JSON_OBJECT_SIZE(4)> innerDoc;
  innerDoc["string"] = "exampleResponseString";
  innerDoc["int"] = 5;
  innerDoc["float"] = 5.0f;
  innerDoc["bool"] = true;
  response["json_data"] = innerDoc;
}

const RPC_Callback xcallbacks[3] = {
    {"USER_CONTROL_01", RPC_TEST_process1},
    {"USER_CONTROL_02", RPC_TEST_process2},
    {"USER_CONTROL_03", RPC_TEST_process3}};
void setup() {
    Serial.begin(9600);
    delay(1000); 
    Serial.println("\n\n===== System Booting Up =====");
    cliManager = new CommandLineManager(nullptr, &preferences);
    cliManager->loadSettings();
    Serial.println("--- Loaded Stored Settings ---");
    Serial.print("  SSID: "); Serial.println(cliManager->getSsid());
    Serial.print("  Server: "); Serial.println(cliManager->getServer());
    Serial.print("  Token: "); Serial.println(cliManager->getToken());
    Serial.println("------------------------------");
    tbManager = new TBmanager(
        cliManager->getSsid(),
        cliManager->getPassword(),
        cliManager->getServer(),
        cliManager->getToken()
    );
    cliManager = new CommandLineManager(tbManager, &preferences);
    cliManager->loadSettings(); 
    tbManager->RPCRoute(xcallbacks);
    if (cliManager->shouldEnterMenuOnBoot(10000)) {
       Serial.println("\n>> ผู้ใช้ขัดจังหวะ, เข้าสู่เมนูตั้งค่า");
    } else {
        Serial.println("\n>> หมดเวลา, กำลังเริ่มการเชื่อมต่อเบื้องหลัง...");
         cliManager->begin();
        tbManager->begin(); 
    }
}
void loop() { 
    vTaskDelay(pdMS_TO_TICKS(1000)); 
}
