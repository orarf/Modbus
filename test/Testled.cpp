#include <Arduino.h>
#define LED1 23  // กำหนดขาที่จะเทส
#define LED2 19
#define LED3 18
#define LED4 5
void setup() {
  Serial.begin(9600);
  pinMode(LED1, OUTPUT);  // ตั้งเป็นขาออก
    pinMode(LED2, OUTPUT);
    pinMode(LED3, OUTPUT);
    pinMode(LED4, OUTPUT);
  Serial.println("Starting test on GPIO23...");
}

void loop() {
  digitalWrite(LED1, HIGH); // เปิดไฟ
  digitalWrite(LED2, HIGH);
  digitalWrite(LED3, HIGH);
    digitalWrite(LED4, HIGH);
  Serial.println("GPIO23 HIGH");
  delay(500);                   // หน่วง 500ms
  digitalWrite(LED1, LOW);  // ปิดไฟ
    digitalWrite(LED2, LOW);
    digitalWrite(LED3, LOW);
    digitalWrite(LED4, LOW);
  Serial.println("GPIO23 LOW");
  delay(500);                   // หน่วง 500ms
}
