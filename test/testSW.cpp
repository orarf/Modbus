#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#define LED_PIN 22  // ใช้ GPIO2 ซึ่งมักต่อกับ LED บนบอร์ด ESP32

String inputString = "";  // สำหรับเก็บข้อความที่รับมาจาก Serial

void setup() {
  Serial.begin(9600);         // เริ่ม Serial Monitor ที่ baudrate 115200
  pinMode(LED_PIN, OUTPUT);     // ตั้งขา LED เป็น output
  digitalWrite(LED_PIN, LOW);   // เริ่มต้นให้ LED ดับ
  Serial.println("พิมพ์ 'on' หรือ 'off' เพื่อสั่งไฟ");
}

void loop() {
  if (Serial.available()) {
    inputString = Serial.readStringUntil('\n');  // อ่านค่าจนกว่าจะเจอ Enter
    inputString.trim(); // ลบช่องว่างหรือ \r\n ด้านหน้า-หลัง

    if (inputString == "on") {
      digitalWrite(LED_PIN, HIGH);  // เปิดไฟ
      Serial.println("LED ติดแล้ว");
    } else if (inputString == "off") {
      digitalWrite(LED_PIN, LOW);   // ดับไฟ
      Serial.println("LED ดับแล้ว");
    } else {
      Serial.println("พิมพ์แค่ 'on' หรือ 'off' เท่านั้นนะ");
    }
  }
}
