#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/queue.h"
void setup() {
  Serial.begin(9600);
  Serial.println("Ready to receive commands:");
}

void loop() {
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    Serial.print("Received: ");
    Serial.println(line);
  }
}