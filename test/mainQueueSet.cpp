#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// ---------- Global Definitions ----------
QueueHandle_t queueSensorA;         // Queue สำหรับ Sensor A
QueueHandle_t queueSensorB;         // Queue สำหรับ Sensor B
QueueSetHandle_t sensorQueueSet;    // Queue Set ที่รวม Queue ทั้งสอง

// ---------- Task: ส่งข้อมูลเข้า Queue A ----------
void vSensorATask(void *pvParameters) {
  int value = 0;
  while (1) {
    value++;
    xQueueSend(queueSensorA, &value, portMAX_DELAY); // ส่งค่าลง Queue A
    vTaskDelay(pdMS_TO_TICKS(1000)); // ส่งทุก 1 วินาที
  }
}

// ---------- Task: ส่งข้อมูลเข้า Queue B ----------
void vSensorBTask(void *pvParameters) {
  int value = 1000;
  while (1) {
    value++;
    xQueueSend(queueSensorB, &value, portMAX_DELAY); // ส่งค่าลง Queue B
    vTaskDelay(pdMS_TO_TICKS(1500)); // ส่งทุก 1.5 วินาที
  }
}

// ---------- Task: อ่านค่าจาก Queue Set ----------
void vReceiverTask(void *pvParameters) {
  QueueHandle_t activatedQueue;
  int receivedValue;

  while (1) {
    // รอ queue ใด queue หนึ่งใน set มีข้อมูล
    activatedQueue = xQueueSelectFromSet(sensorQueueSet, portMAX_DELAY);

    if (activatedQueue == queueSensorA) {
      xQueueReceive(queueSensorA, &receivedValue, 0); // อ่านค่าจาก Queue A
      Serial.printf("[Receiver] Got from Sensor A: %d\n", receivedValue);
    }
    else if (activatedQueue == queueSensorB) {
      xQueueReceive(queueSensorB, &receivedValue, 0); // อ่านค่าจาก Queue B
      Serial.printf("[Receiver] Got from Sensor B: %d\n", receivedValue);
    }
  }
}

// ---------- Setup ----------
void setup() {
  Serial.begin(9600);

  // สร้าง queue สำหรับ sensor A และ B
  queueSensorA = xQueueCreate(5, sizeof(int));  // ความจุ 5 item
  queueSensorB = xQueueCreate(5, sizeof(int));

  // สร้าง queue set (รวมได้ 10 item = 5 จาก A + 5 จาก B)
  sensorQueueSet = xQueueCreateSet(10);

  // ใส่ queue A และ B เข้าไปใน set
  xQueueAddToSet(queueSensorA, sensorQueueSet);
  xQueueAddToSet(queueSensorB, sensorQueueSet);

  // สร้าง Task สำหรับ Sensor A, B และ Receiver
  xTaskCreate(vSensorATask, "Sensor A", 2048, NULL, 1, NULL);
  xTaskCreate(vSensorBTask, "Sensor B", 2048, NULL, 1, NULL);
  xTaskCreate(vReceiverTask, "Receiver", 4096, NULL, 2, NULL);
}

void loop() {
  // ไม่ต้องทำอะไรใน loop
}
