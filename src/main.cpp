#include <ModbusMaster.h>

#define MAX485_DE 4  // GPIO ควบคุม DE/RE ของ MAX485
#define MAX485_RE 4  // ต่อร่วมกัน

ModbusMaster node;

// ตัวแปร global สำหรับ slaveId และ baud
uint8_t slaveId = 1;
uint32_t baud = 9600;

void preTransmission() { digitalWrite(MAX485_DE, HIGH); }
void postTransmission() { digitalWrite(MAX485_DE, LOW); }

// ฟังก์ชันสแกนหา Modbus ID และ baud rate
bool scanModbusDevice(uint8_t &foundId, uint32_t &foundBaud) {
  uint32_t baudRates[] = {9600, 19200, 38400, 115200};
  Serial.println("\n========== Scan Begin Modbus ==========");
  for (int b = 0; b < 4; b++) {
    Serial.print("\n[Baud Rate] ");
    Serial.println(baudRates[b]);
    Serial2.begin(baudRates[b], SERIAL_8N1, 16, 17);
    for (uint8_t id = 1; id <= 247; id++) {
      Serial.print("  [Scanning] ID: ");
      Serial.print(id);
      Serial.print(" ... ");
      node.begin(id, Serial2);
      node.preTransmission(preTransmission);
      node.postTransmission(postTransmission);
      uint8_t result = node.readHoldingRegisters(0x0000, 1);
      if (result == node.ku8MBSuccess) {
        Serial.println("Found Device!");
        foundId = id;
        foundBaud = baudRates[b];
        Serial.print("\n========== Found ID: ");
        Serial.print(id);
        Serial.print(" Baud Rate: ");
        Serial.print(baudRates[b]);
        Serial.println(" ==========\n");
        return true;
      } else {
        Serial.println("Not Found");
      }
      vTaskDelay(5 / portTICK_PERIOD_MS);
    }
  }
  Serial.println("\n========== Not Found Device Modbus ==========\n");
  return false;
}

// Task สำหรับอ่านค่า Modbus
void TaskModbus(void *pvParameters) {
  int failCount = 0;
  for (;;) {
    uint8_t result = node.readHoldingRegisters(0x0000, 10);
    if (result == node.ku8MBSuccess) {
      Serial.println("Read Holding Registers success:");
      for (int i = 0; i < 10; i++) {
        Serial.print("Register ");
        Serial.print(i);
        Serial.print(": ");
        Serial.println(node.getResponseBuffer(i));
      }
      failCount = 0; // reset เมื่ออ่านสำเร็จ
    } else {
      Serial.print("Read fail. Error code: ");
      Serial.println(result);
      failCount++;
      if (failCount >= 5) { // ถ้าอ่านไม่สำเร็จ 5 ครั้ง
        Serial.println("Lost connection. Start scanning again...");
        if (scanModbusDevice(slaveId, baud)) {
          Serial.println("Auto select Modbus device.");
          Serial2.begin(baud, SERIAL_8N1, 16, 17);
          node.begin(slaveId, Serial2);
        } else {
          Serial.println("No Modbus device found. Use default.");
          Serial2.begin(9600, SERIAL_8N1, 16, 17);
          node.begin(1, Serial2);
        }
        failCount = 0; // reset counter หลังสแกนใหม่
      }
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(MAX485_DE, OUTPUT);
  pinMode(MAX485_RE, OUTPUT);
  postTransmission();

  // สแกนหาอุปกรณ์และตั้งค่าอัตโนมัติ
  if (scanModbusDevice(slaveId, baud)) {
    Serial.println("Auto select Modbus device.");
    Serial2.begin(baud, SERIAL_8N1, 16, 17);
    node.begin(slaveId, Serial2);
  } else {
    Serial.println("No Modbus device found. Use default.");
    Serial2.begin(9600, SERIAL_8N1, 16, 17);
    node.begin(1, Serial2);
  }

  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  xTaskCreate(
    TaskModbus,
    "ModbusTask",
    4096,
    NULL,
    1,
    NULL
  );
}

void loop() {
  // ว่างไว้ เพราะเราจะทำงานผ่าน Task
}