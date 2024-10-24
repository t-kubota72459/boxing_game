#include "M5AtomS3.h"
#include "microbit.h"
#include "M5AtomicMotion.h"

#define MY_PEER "C2:FD:63:8F:CC:42"

volatile bool interruptFlag = false;

void IRAM_ATTR handleInterrupt()
{
  interruptFlag = true;
}

M5AtomicMotion AtomicMotion;
microbit mymb = microbit();
QueueHandle_t queue;

// Motor mymotor = Motor();

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

  pinMode(1, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(1), handleInterrupt, FALLING);

  AtomS3.Display.setTextColor(GREEN);
  AtomS3.Display.setTextDatum(middle_center);
  AtomS3.Display.setTextSize(2);
  AtomS3.Display.drawString("Atomic Init", AtomS3.Display.width() / 2, AtomS3.Display.height() / 2);

  // for AtomS3:
  //    sda=38
  //    scl=39
  //
  while (!AtomicMotion.begin(&Wire, M5_ATOMIC_MOTION_I2C_ADDR, 38, 39, 100000)) {
    AtomS3.Display.clear();
    AtomS3.Display.drawString("Init Fail", AtomS3.Display.width() / 2, AtomS3.Display.height() / 2);
  }
  AtomS3.Display.clear();
  AtomS3.Display.drawString("OK", M5.Display.width() / 2, M5.Display.height() / 2);

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

  if (interruptFlag) {
    AtomS3.Display.clear();
    AtomS3.Display.setTextColor(RED);
    AtomS3.Display.drawString("HIT!!", AtomS3.Display.width() / 2, AtomS3.Display.height() / 2);
    Serial.println("HIT!!::stop");
    
    AtomicMotion.setMotorSpeed(0, 0);
    AtomicMotion.setMotorSpeed(1, 0);
    delay(1000);  // stop for 1 seconds.
    Serial.println("HIT!!::release");
    xQueueReset(queue);
    AtomS3.Display.clear();
    AtomS3.Display.setTextColor(GREEN);
    AtomS3.Display.drawString("FIGHT!!", AtomS3.Display.width() / 2, AtomS3.Display.height() / 2);
    interruptFlag = false;
  }

  xQueueReceive(queue, &s, portMAX_DELAY);
  Serial.print(s.buttons);
  Serial.print(",");
  Serial.print(s.accel[0]);
  Serial.print(",");
  Serial.print(s.accel[1]);
  Serial.print(",");
  Serial.println(s.accel[2]);

  int y = map(s.accel[1], -1100, +1100, -110, +110);
  int x = map(s.accel[0], -1100, +1100, -1, +2);
  Serial.print("y=");
  Serial.println(y);
  Serial.print("x=");
  Serial.println(x);
  
  int x0 = 1 - max(0, x);
  int x1 = 1 - max(0, -x);

  int y0 = -int(y * x0);
  int y1 = -int(y * x1);
  
  Serial.print(y0);
  Serial.print(",");
  Serial.println(y1);

  if (0xF0 & s.buttons) {
    AtomicMotion.setMotorSpeed(0, 80);
    AtomicMotion.setMotorSpeed(1, -80);
  } else if (0x0F & s.buttons) {
    AtomicMotion.setMotorSpeed(0, -80);
    AtomicMotion.setMotorSpeed(1, 80);
  } else if (x == 0) {
    AtomicMotion.setMotorSpeed(0, y0);
    AtomicMotion.setMotorSpeed(1, y1);
  } else if ( x > 0) {
    AtomicMotion.setMotorSpeed(0, 100);
    AtomicMotion.setMotorSpeed(1, 0);
  } else if ( x < 0) {
    AtomicMotion.setMotorSpeed(0, 0);
    AtomicMotion.setMotorSpeed(1, 100);
  }

  delay(10);
}
