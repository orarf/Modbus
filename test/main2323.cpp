// Relay control - ESP32 (Serial commands: on / off / toggle / status)
#include <Arduino.h>

const int RELAY_PIN = 23 ; // เปลี่ยนเป็นขาที่ต่อรีเลย์ของคุณ

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  Serial.begin(9600);
  
}

void loop() {
  digitalWrite(RELAY_PIN, HIGH); // เปิดรีเลย์
  Serial.println("Relay is ON");
  delay(1000);                   // รอ 1 วินาที
  digitalWrite(RELAY_PIN, LOW);  // ปิดรีเลย์   
  Serial.println("Relay is OFF"); 
  delay(1000);  
}
