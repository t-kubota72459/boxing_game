#include "M5AtomS3.h"
#include "microbit.h"

#define MY_PEER "C2:FD:63:8F:CC:42"

microbit mymb = microbit();
QueueHandle_t queue;

void task1(void* pvParameters) {
    sensor_data s;
    while (1) {
      s.buttons = (uint16_t)(((uint8_t)mymb.get_a_button()) << 4 | ((uint8_t)mymb.get_b_button()));
      mymb.get_accelerometer(s.accel);
      xQueueSend(queue, &s, 1);
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

  queue = xQueueCreate(10, sizeof(sensor_data));
  xTaskCreatePinnedToCore(task1, "task1", 4096, NULL, 1, NULL, 1);
}

void loop()
{
  sensor_data s;
  AtomS3.update();
  xQueueReceive(queue, &s, portMAX_DELAY);
  Serial.print(s.buttons);
  Serial.print(",");
  Serial.print(s.accel[0]);
  Serial.print(",");
  Serial.print(s.accel[1]);
  Serial.print(",");
  Serial.println(s.accel[2]);
  delay(10);
}
