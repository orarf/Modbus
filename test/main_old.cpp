// This sketch demonstrates connecting and receiving RPC calls from 
// ThingsBoard using ThingsBoard SDK.
//
// Hardware:
//  - Arduino Uno
//  - ESP8266 connected to Arduino Uno
#include <WiFi.h>
#include <TBmanager.h>




// Define The Connection info 
#define USER_WIFI_SSID "Apimo"
#define USER_WIFI_PASSWORD  "wwssadad"
#define TB_SERVER "thingsboard.tricommtha.com"
#define TB_TOKEN "s4WRgQPHg2p1dzEjwU3C"  // Device TOKEN ID

TBmanager tbClient(USER_WIFI_SSID, USER_WIFI_PASSWORD,TB_SERVER,TB_TOKEN);






void RPC_TEST_process1(const JsonVariantConst &data, JsonDocument &response) {
  Serial.println("Received the json RPC method RPC_TEST_process1");

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



void setup(){
  vDelay_ms(100);
  Serial.begin(9600);



  tbClient.RPCRoute(xcallbacks);
  vDelay_ms(100);
  tbClient.begin();
  vDelay_ms(2000);


}
void loop(){
  // delay(1000);
  // Serial.println("DEV_SEND_TELEMETRY");

}