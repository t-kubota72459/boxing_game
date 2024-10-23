#include "M5AtomS3.h"
#include "microbit.h"

#define MY_PEER "C2:FD:63:8F:CC:42"

microbit mymb = microbit();
QueueHandle_t queue;

void task1(void* pvParameters) {
    uint16_t b;
    while (1) {
      b = (uint16_t)(((uint8_t)mymb.get_a_button()) << 8 | ((uint8_t)mymb.get_b_button()));
      xQueueSend(queue, &b, 1);
      delay(20);
    }
}

void setup()
{
  auto cfg = M5.config();
  AtomS3.begin(cfg);
  Serial.begin(115200);
  microbit::init();
  microbit::connect(&mymb, MY_PEER);

  queue = xQueueCreate(10, 2);
  xTaskCreatePinnedToCore(task1, "task1", 4096, NULL, 1, NULL, 1);
}

void loop()
{
  uint16_t b;
  AtomS3.update();
  xQueueReceive(queue, &b, portMAX_DELAY);
  Serial.println("Hello world!");
  Serial.println(b);
  delay(10);
}
