#include <Arduino.h>
#include <Preferences.h>
#include "TBmanager.h"
#include "commandline.h"
#include "ModbusRTUMaster.h"

// ---------------- Modbus ----------------
#define MAX485_DE 4
#define MAX485_RE 4
#define LED1 23 // กำหนดขาที่จะเทส
#define LED2 19
#define LED3 18
#define LED4 5
#define SOIL_PIN 34
Preferences prefs;
SemaphoreHandle_t xMutex;    // สร้าง Mutex
bool ledShouldBeOn1 = false; // ตัวแปรที่ใช้ร่วมกัน
bool ledShouldBeOn2 = false;
bool ledShouldBeOn3 = false;
bool ledShouldBeOn4 = false;
HardwareSerial RS485(2); // Serial2 (RX=16, TX=17)
ModbusRTUMaster modbus(RS485, MAX485_DE, MAX485_RE);
uint8_t slaveId = 3;
uint8_t slaveId2 = 4; // Modbus Slave ID
uint32_t baud = 4800;

void preTransmission()
{
  digitalWrite(MAX485_DE, HIGH);
  digitalWrite(MAX485_RE, HIGH);
  delayMicroseconds(100);
}

void postTransmission()
{
  digitalWrite(MAX485_DE, LOW);
  digitalWrite(MAX485_RE, LOW);
  delayMicroseconds(100);
}

// ---------------- RPC Example ----------------
TBmanager *tbManager;
CommandLineManager *cliManager;
Preferences preferences;

void RPC_TEST_process1(const JsonVariantConst &data, JsonDocument &response)
{
  if (xSemaphoreTake(xMutex, portMAX_DELAY))
  {
    if (data == "on1")
    {
      ledShouldBeOn1 = true; // เปิดไฟ
    }
    else if (data == "off1")
    {
      ledShouldBeOn1 = false; // ดับไฟ
    }
    else if (data == "on2")
    {
      ledShouldBeOn2 = true; // ดับไฟ
    }
    else if (data == "off2")
    {
      ledShouldBeOn2 = false; // ดับไฟ
    }
    else if (data == "on3")
    {
      ledShouldBeOn3 = true; // ดับไฟ
    }
    else if (data == "off3")
    {
      ledShouldBeOn3 = false; // ดับไฟ
    }
    else if (data == "on4")
    {
      ledShouldBeOn4 = true; // ดับไฟ
    }
    else if (data == "off4")
    {
      ledShouldBeOn4 = false; // ดับไฟ
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

void RPC_TEST_process2(const JsonVariantConst &data, JsonDocument &response)
{
  Serial.println("Received RPC_TEST_process2");
  StaticJsonDocument<JSON_OBJECT_SIZE(4)> inner;
  inner["string"] = "exampleResponseString RPC_TEST_process2";
  inner["int"] = 5;
  inner["float"] = 5.0f;
  inner["bool"] = true;
  response["json_data"] = inner;
}

void RPC_TEST_process3(const JsonVariantConst &data, JsonDocument &response)
{
  Serial.println("Received RPC_TEST_process3");
  StaticJsonDocument<JSON_OBJECT_SIZE(4)> inner;
  inner["string"] = "exampleResponseString";
  inner["int"] = 5;
  inner["float"] = 5.0f;
  inner["bool"] = true;
  response["json_data"] = inner;
}

const RPC_Callback xcallbacks[3] = {
    {"USER_CONTROL_01", RPC_TEST_process1},
    {"USER_CONTROL_02", RPC_TEST_process2},
    {"USER_CONTROL_03", RPC_TEST_process3}};

void npkCallback(TimerHandle_t xTimer)
{
  uint16_t nitrogen = 0, phosphorus = 0, potassium = 0;
  uint16_t rawTemp = 0;
  uint16_t rawLight = 0;
  uint16_t rawHum = 0;
  int rawValue = analogRead(SOIL_PIN);
  int soilPercent = map(rawValue, 4095, 1500, 0, 100);
  tbManager->sendAttributeData("SoilMoistureRaw", rawValue); // ค่าดิบ
  tbManager->sendAttributeData("SoilMoisture", soilPercent); // ค่าเป็นเปอร์เซ็นต์
  tbManager->sendTelemetryData("SoilMoistureRaw", rawValue); // ค่าดิบ
  tbManager->sendTelemetryData("SoilMoisture", soilPercent); // ค่าเป็นเqปอร์เซ็นต์

  preTransmission();
  ModbusRTUMasterError errN = modbus.readHoldingRegisters(slaveId, 13, &nitrogen, 1);
  ModbusRTUMasterError errP = modbus.readHoldingRegisters(slaveId, 15, &phosphorus, 1);
  ModbusRTUMasterError errK = modbus.readHoldingRegisters(slaveId, 16, &potassium, 1);
  ModbusRTUMasterError errTemp = modbus.readHoldingRegisters(slaveId2, 1, &rawTemp, 1);   // register 1 = temp
  ModbusRTUMasterError errLight = modbus.readHoldingRegisters(slaveId2, 2, &rawLight, 1); // register 2 = light
  ModbusRTUMasterError errHum = modbus.readHoldingRegisters(slaveId2, 0, &rawHum, 1);     // register 3 = humidity
  postTransmission();

  if (errTemp == MODBUS_RTU_MASTER_SUCCESS && errLight == MODBUS_RTU_MASTER_SUCCESS && errHum == MODBUS_RTU_MASTER_SUCCESS)
  {
    float temperature = rawTemp / 10.0;
    float humidity = rawHum / 10.0;
    float light = rawLight * 1.0;

    tbManager->sendTelemetryData("Temperature", temperature);
    tbManager->sendTelemetryData("Humidity", humidity);
    tbManager->sendTelemetryData("Light", light);
    tbManager->sendAttributeData("Temperature", temperature);
    tbManager->sendAttributeData("Humidity", humidity);
    tbManager->sendAttributeData("Light", light);
  }
  else
  {
    Serial.printf("Modbus Error Temp:%d Light:%d Hum:%d\n", errTemp, errLight, errHum);
  }
  if (errN == MODBUS_RTU_MASTER_SUCCESS && errP == MODBUS_RTU_MASTER_SUCCESS && errK == MODBUS_RTU_MASTER_SUCCESS)
  {
    tbManager->sendTelemetryData("Nitrogen", nitrogen);
    tbManager->sendTelemetryData("Phosphorus", phosphorus);
    tbManager->sendTelemetryData("Potassium", potassium);
    tbManager->sendAttributeData("Nitrogen", nitrogen);
    tbManager->sendAttributeData("Phosphorus", phosphorus);
    tbManager->sendAttributeData("Potassium", potassium);
  }
  else
  {
    Serial.printf("Modbus Error N:%d P:%d K:%d\n", errN, errP, errK);
  }
}

void TaskLED(void *pvParameters)
{
  bool previousState1 = !ledShouldBeOn1;
  bool previousState2 = !ledShouldBeOn2;
  bool previousState3 = !ledShouldBeOn3;
  bool previousState4 = !ledShouldBeOn4;

  prefs.begin("led_states", false); // เปิด preferences (read-write)

  while (1)
  {
    bool currentState1;
    bool currentState2;
    bool currentState3;
    bool currentState4;

    if (xSemaphoreTake(xMutex, portMAX_DELAY))
    {
      currentState1 = ledShouldBeOn1;
      currentState2 = ledShouldBeOn2;
      currentState3 = ledShouldBeOn3;
      currentState4 = ledShouldBeOn4;
      xSemaphoreGive(xMutex);
    }

    // เขียนค่าออกไปยัง GPIO
    digitalWrite(LED1, currentState1 ? HIGH : LOW);
    digitalWrite(LED2, currentState2 ? HIGH : LOW);
    digitalWrite(LED3, currentState3 ? HIGH : LOW);
    digitalWrite(LED4, currentState4 ? HIGH : LOW);

    // บันทึกค่าเฉพาะเมื่อมีการเปลี่ยนแปลง
    if (currentState1 != previousState1)
    {
      prefs.putBool("led1", currentState1);
      previousState1 = currentState1;
    }
    if (currentState2 != previousState2)
    {
      prefs.putBool("led2", currentState2);
      previousState2 = currentState2;
    }
    if (currentState3 != previousState3)
    {
      prefs.putBool("led3", currentState3);
      previousState3 = currentState3;
    }
    if (currentState4 != previousState4)
    {
      prefs.putBool("led4", currentState4);
      previousState4 = currentState4;
    }

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }

  prefs.end(); // (จริง ๆ ไม่จำเป็นใน loop ไม่จบ แต่ใส่ไว้เผื่อ copy ไปใช้ที่อื่น)
}

// ---------------- Setup ----------------
void setup()
{
  Serial.begin(9600);
  prefs.begin("led_states", true); // true = read-only
  ledShouldBeOn1 = prefs.getBool("led1", false);
  ledShouldBeOn2 = prefs.getBool("led2", false);
  ledShouldBeOn3 = prefs.getBool("led3", false);
  ledShouldBeOn4 = prefs.getBool("led4", false);
  prefs.end();
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(LED4, OUTPUT);
  xMutex = xSemaphoreCreateMutex();

  pinMode(MAX485_DE, OUTPUT);
  pinMode(MAX485_RE, OUTPUT);
  postTransmission();

  RS485.begin(baud, SERIAL_8N1, 16, 17);
  modbus.begin(baud, SERIAL_8N1);

  Serial.println("\n===== System Booting Up =====");

  // โหลดค่าเก่าก่อน
  cliManager = new CommandLineManager(nullptr, &preferences);
  cliManager->loadSettings();
  Serial.println("--- Loaded Stored Settings ---");
  Serial.print("  SSID: ");
  Serial.println(cliManager->getSsid());
  Serial.print("  Server: ");
  Serial.println(cliManager->getServer());
  Serial.print("  Token: ");
  Serial.println(cliManager->getToken());
  Serial.println("------------------------------");

  // สร้าง TBmanager
  tbManager = new TBmanager(
      cliManager->getSsid(),
      cliManager->getPassword(),
      cliManager->getServer(),
      cliManager->getToken());

  // เชื่อม CLI กับ TBmanager
  cliManager = new CommandLineManager(tbManager, &preferences);
  cliManager->loadSettings();

  // Register RPC callbacks
  tbManager->RPCRoute(xcallbacks);

  // ตรวจสอบว่ากดเข้าเมนูตั้งค่าไหม
  if (cliManager->shouldEnterMenuOnBoot(10000))
  {
    Serial.println("\n>> User entered setup menu");
    cliManager->begin();
  }
  else
  {
    Serial.println("\n>> Booting automatic connection...");
    cliManager->begin();
    tbManager->begin();
  }
  TimerHandle_t npkTimer = xTimerCreate("npkTimer", pdMS_TO_TICKS(60000), pdTRUE, (void *)0, npkCallback);
  // เริ่ม Timer
  if (npkTimer != NULL)
    xTimerStart(npkTimer, 0);
  xTaskCreate(TaskLED, "LED Task", 2048, NULL, 1, NULL);
}

// ---------------- Loop ----------------
void loop()
{
  vTaskDelay(pdMS_TO_TICKS(1000));
  // ทำงานผ่าน FreeRTOS task
}
