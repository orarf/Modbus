#include <ModbusRTUMaster.h>

#define MAX485_DE 4
#define MAX485_RE 4

ModbusRTUMaster modbus(Serial2, MAX485_DE, MAX485_RE);

void setup() {
  Serial.begin(115200);
  pinMode(MAX485_DE, OUTPUT);
  pinMode(MAX485_RE, OUTPUT);
  digitalWrite(MAX485_DE, LOW);
  digitalWrite(MAX485_RE, LOW);

  Serial2.begin(4800, SERIAL_8N1, 16, 17); // RX=16, TX=17
  modbus.begin(4800, SERIAL_8N1);
  modbus.setTimeout(500); // เพิ่ม timeout ถ้าตอบช้า
}

void loop() {
  uint16_t soilMoisture;
  ModbusRTUMasterError err = modbus.readHoldingRegisters(3, 14, &soilMoisture, 1); // ID=3, address=13, จำนวน=1

  if (err == MODBUS_RTU_MASTER_SUCCESS) {
    Serial.print("Soil Moisture: ");
    Serial.println(soilMoisture);
  } else {
    Serial.print("Modbus error: ");
    Serial.println(err);
  }
  delay(1000);
}