#include <Arduino.h>
#include "ModbusRTUMaster.h"

#define MAX485_DE  4   // GPIO ควบคุม DE ของ MAX485
#define MAX485_RE  4   // GPIO ควบคุม RE ของ MAX485

HardwareSerial RS485(2);      // Serial2 (RX=16, TX=17)
ModbusRTUMaster modbus(RS485, MAX485_DE, MAX485_RE);

uint8_t slaveId = 3;          // Slave ID
uint32_t baud = 4800;         // Baud rate

// ควบคุม DE/RE
void preTransmission() {
  digitalWrite(MAX485_DE, HIGH);
  digitalWrite(MAX485_RE, HIGH);
  delayMicroseconds(100); // เพิ่มหน่วงเล็กน้อย
}

void postTransmission() {
  digitalWrite(MAX485_DE, LOW);
  digitalWrite(MAX485_RE, LOW);
  delayMicroseconds(100); // เพิ่มหน่วงเล็กน้อย
}

// Task อ่านค่า Modbus
void readTask(void *pvParameters) {
  uint16_t buf[10];
  for (;;) {
    preTransmission();
    uint16_t soilMoisture;
    ModbusRTUMasterError err = modbus.readHoldingRegisters(slaveId, 13, &soilMoisture, 1); // ID=3, address=13, จำนวน=1
    postTransmission();

   if (err == MODBUS_RTU_MASTER_SUCCESS) {
    Serial.print("Soil Moisture: ");
    Serial.println(soilMoisture);
  } else {
    Serial.print("Modbus error: ");
    Serial.println(err);
  }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(1000);
  pinMode(MAX485_DE, OUTPUT);
  pinMode(MAX485_RE, OUTPUT);
  postTransmission(); // เริ่มต้นโหมดรับ

  RS485.begin(baud, SERIAL_8N1, 16, 17);
  modbus.begin(baud, SERIAL_8N1);

  Serial.println("Starting Modbus read from Slave ID 1...");

  xTaskCreate(
    readTask,
    "ModbusReadTask",
    4096,
    NULL,
    1,
    NULL
  );
}

void loop() {
  // ทำงานผ่าน FreeRTOS task
}
