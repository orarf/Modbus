#include <Arduino.h>
#include "ModbusRTUMaster.h"

#define MAX485_DE  4
#define MAX485_RE  4

HardwareSerial RS485(2);  // Serial2 (RX=16, TX=17)
ModbusRTUMaster modbus(RS485, MAX485_DE, MAX485_RE);

uint8_t slaveId = 3;      // Slave ID
uint32_t baud = 4800;     // Baud rate

void preTransmission() {
  digitalWrite(MAX485_DE, HIGH);
  digitalWrite(MAX485_RE, HIGH);
  delayMicroseconds(100);
}

void postTransmission() {
  digitalWrite(MAX485_DE, LOW);
  digitalWrite(MAX485_RE, LOW);
  delayMicroseconds(100);
}

void readTask(void *pvParameters) {
  for (;;) {
    uint16_t nitrogen, phosphorus, potassium;

    preTransmission();
    ModbusRTUMasterError errN = modbus.readHoldingRegisters(slaveId, 13, &nitrogen, 1);
    ModbusRTUMasterError errP = modbus.readHoldingRegisters(slaveId, 15, &phosphorus, 1);
    ModbusRTUMasterError errK = modbus.readHoldingRegisters(slaveId, 16, &potassium, 1);
    postTransmission();

    if (errN == MODBUS_RTU_MASTER_SUCCESS &&
        errP == MODBUS_RTU_MASTER_SUCCESS &&
        errK == MODBUS_RTU_MASTER_SUCCESS) {

      Serial.print("Nitrogen (N): "); Serial.println(nitrogen);
      Serial.print("Phosphorus (P): "); Serial.println(phosphorus);
      Serial.print("Potassium (K): "); Serial.println(potassium);
      Serial.println("------");
    } else {
      Serial.print("Modbus error N:"); Serial.print(errN);
      Serial.print(" P:"); Serial.print(errP);
      Serial.print(" K:"); Serial.println(errK);
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(MAX485_DE, OUTPUT);
  pinMode(MAX485_RE, OUTPUT);
  postTransmission();

  RS485.begin(baud, SERIAL_8N1, 16, 17);
  modbus.begin(baud, SERIAL_8N1);

  Serial.println("Starting Modbus read from Slave ID 3...");

  xTaskCreate(readTask, "ModbusReadTask", 4096, NULL, 1, NULL);
}

void loop() {
  // ทำงานผ่าน FreeRTOS task
}
