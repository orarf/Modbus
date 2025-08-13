#include <ModbusMaster.h>

#define MAX485_DE 4  // GPIO ควบคุม DE/RE ของ MAX485
#define MAX485_RE 4  // ต่อร่วมกัน

ModbusMaster node;

void preTransmission() { digitalWrite(MAX485_DE, HIGH); }
void postTransmission() { digitalWrite(MAX485_DE, LOW); }

// Task สำหรับอ่านค่า Modbus
void TaskModbus(void *pvParameters) {
  for (;;) {  // วนลูปไม่รู้จบ
    uint8_t result = node.readHoldingRegisters(0x0000, 10);  // อ่าน 10 register

    if (result == node.ku8MBSuccess) {
      Serial.println("Read Holding Registers success:");
      for (int i = 0; i < 10; i++) {
        Serial.print("Register ");
        Serial.print(i);
        Serial.print(": ");
        Serial.println(node.getResponseBuffer(i));
      }
    } else {
      Serial.print("Read fail. Error code: ");
      Serial.println(result);
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);  // หน่วง 1 วินาที
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(MAX485_DE, OUTPUT);
  pinMode(MAX485_RE, OUTPUT);
  postTransmission();

  Serial2.begin(9600, SERIAL_8N1, 16, 17); // UART2 RX=16, TX=17

  node.begin(1, Serial2);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  // สร้าง FreeRTOS Task
  xTaskCreate(
    TaskModbus,    // ชื่อฟังก์ชัน Task
    "ModbusTask",  // ชื่อ Task
    4096,          // ขนาด stack
    NULL,          // พารามิเตอร์ส่งเข้า Task
    1,             // Priority
    NULL           // ตัว handle
  );
}

void loop() {
  // ว่างไว้ เพราะเราจะทำงานผ่าน Task
}