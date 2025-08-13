#include <WiFi.h>
#include <ArduinoJson.h>   // <-- เพิ่มบรรทัดนี้เข้ามา
#include "TBmanager.h"     // <-- แก้ไขจาก <TBmanager.h> เป็น "TBmanager.h" ถ้าไฟล์อยู่ในโปรเจกต์
#include "commandline.h"   // <-- แก้ไขจาก <commandline.h> เป็น "commandline.h"

// Global pointers to our main objects
TBmanager* tbClient;
CommandLine* cli;

// --- ฟังก์ชันสำหรับรับคำสั่ง RPC ย้ายมาอยู่ที่นี่ ---
void RPC_TEST_process1(const JsonVariantConst &data, JsonDocument &response) {
  Serial.println("Received the json RPC method RPC_TEST_process1");
  StaticJsonDocument<JSON_OBJECT_SIZE(4)> innerDoc;
  innerDoc["string"] = "exampleResponseString";
  innerDoc["int"] = 5;
  innerDoc["float"] = 5.0f;
  innerDoc["bool"] = true;
  response["json_data"] = innerDoc;
}

void RPC_TEST_process2(const JsonVariantConst &data, JsonDocument &response) {
  Serial.println("Received the json RPC method RPC_TEST_process2");
  StaticJsonDocument<JSON_OBJECT_SIZE(4)> innerDoc;
  innerDoc["string"] = "exampleResponseString RPC_TEST_process2";
  innerDoc["int"] = 5;
  innerDoc["float"] = 5.0f;
  innerDoc["bool"] = true;
  response["json_data"] = innerDoc;
}

void RPC_TEST_process3(const JsonVariantConst &data, JsonDocument &response) {
  Serial.println("Received the json RPC method RPC_TEST_process3 ");
  StaticJsonDocument<JSON_OBJECT_SIZE(4)> innerDoc;
  innerDoc["string"] = "exampleResponseString";
  innerDoc["int"] = 5;
  innerDoc["float"] = 5.0f;
  innerDoc["bool"] = true;
  response["json_data"] = innerDoc;
}

// ตัวแปร xcallbacks ก็ต้องอยู่ที่นี่เช่นกัน
const RPC_Callback xcallbacks[3] = {
    { "USER_CONTROL_01", RPC_TEST_process1 },
    { "USER_CONTROL_02", RPC_TEST_process2 },
    { "USER_CONTROL_03", RPC_TEST_process3 }
};


void setup() {
    Serial.begin(9600);
    delay(100);

    cli = new CommandLine();
    tbClient = new TBmanager(cli->getSsid(), cli->getPassword(), cli->getServer(), cli->getToken());
    cli->setClient(tbClient);
    tbClient->RPCRoute(xcallbacks);
    cli->begin();
}

void loop() {
    vTaskDelete(NULL); 
}