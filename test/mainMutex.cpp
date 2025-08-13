#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

SemaphoreHandle_t xMutex;  // ประกาศตัวแปร Mutex

void Task1(void *pvParameters) {
  while (1) {
    // ขอสิทธิ์ล็อกก่อน
    if (xSemaphoreTake(xMutex, portMAX_DELAY)) {
      Serial.println("👉 Task1: กำลังทำงาน");
      delay(100);  // จำลองการทำงาน
      Serial.println("✅ Task1: เสร็จแล้ว\n");
      xSemaphoreGive(xMutex);  // ปล่อยล็อก
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);  // รอ 500ms แล้วทำใหม่
  }
}

void Task2(void *pvParameters) {
  while (1) {
    if (xSemaphoreTake(xMutex, portMAX_DELAY)) {
      Serial.println("👉 Task2: เริ่มทำงาน");
      delay(100);  // จำลองการทำงาน
      Serial.println("✅ Task2: ทำงานเสร็จ\n");
      xSemaphoreGive(xMutex);
    }
    vTaskDelay(2000 / portTICK_PERIOD_MS);  // รอ 700ms
  }
}

void setup() {
  Serial.begin(9600);
  delay(1000);  // รอ Serial พร้อม

  // สร้าง Mutex
  xMutex = xSemaphoreCreateMutex();

  if (xMutex != NULL) {
    Serial.println("🎯 สร้าง Mutex สำเร็จ");
  }

  // สร้าง Task 2 ตัว
  xTaskCreate(Task1, "Task1", 2048, NULL, 1, NULL);
  xTaskCreate(Task2, "Task2", 2048, NULL, 1, NULL);
}

void loop() {
  // ไม่ต้องทำอะไร
}
