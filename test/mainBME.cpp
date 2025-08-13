#include <Arduino.h>
#define SOIL_PIN 34  // ขาที่ต่อกับ AOUT ของเซนเซอร์

void setup() {
  Serial.begin(9600);
  delay(1000);
}

void loop() {
  int soilValue = analogRead(SOIL_PIN);
  Serial.print("Soil Moisture: ");
  Serial.println(soilValue);
  delay(1000);
}