#include "M5AtomS3.h"
#include "microbit.h"
#include "M5AtomicMotion.h"
#include "Property.h"

//#define MY_PEER "C2:FD:63:8F:CC:42"
#define MY_PEER "D9:1F:4C:D4:A8:35"

volatile bool interruptFlag = false;

Property prop;

void IRAM_ATTR handleInterrupt()
{
  interruptFlag = true;
  prop.DAMAGE++;
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

uint8_t mygetMotorSpeed(uint8_t ch)
{
  uint8_t reg = ch | 0x20;
  return M5.In_I2C.readRegister8(0x38, reg, 400000);
}

uint8_t mysetMotorSpeed(uint8_t ch, int8_t speed)
{
  uint8_t reg = ch + 32;
  Serial.printf("write result=%d\r\n",  M5.In_I2C.writeRegister8(0x38, reg, (uint8_t)speed, 400000));
  return 0;
}

void setup()
{
  auto cfg = M5.config();

  AtomS3.begin(cfg);
  //Wire.begin(M5.Ex_I2C.getSDA(), M5.Ex_I2C.getSCL());

  pinMode(1, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(1), handleInterrupt, FALLING);

  AtomS3.Display.setTextColor(GREEN);
  AtomS3.Display.setTextDatum(middle_center);
  AtomS3.Display.setTextSize(2);
  AtomS3.Display.drawString("Atomic Init", AtomS3.Display.width() / 2, AtomS3.Display.height() / 2);

  // AtomS3.Ex_I2C.
  // for AtomS3:
  //    sda=38
  //    scl=39
  //
  while (!AtomicMotion.begin(&Wire, M5_ATOMIC_MOTION_I2C_ADDR, 38, 39, 100000)) {
    AtomS3.Display.clear();
    AtomS3.Display.drawString("Init Fail", AtomS3.Display.width() / 2, AtomS3.Display.height() / 2);
  }
  AtomS3.Display.clear();
  AtomS3.Display.drawString("OK", AtomS3.Display.width() / 2, AtomS3.Display.height() / 2);

  Serial.begin(115200);
  Serial.printf("M5.getBoard()=%d\r\n", M5.getBoard());
  Serial.printf("M5.Imu.isEnabled()=%d\r\n", M5.Imu.isEnabled());
  Serial.printf("M5.In_I2C.isEnabled()=%d\r\n", M5.In_I2C.isEnabled());
  Serial.printf("M5.Ex_I2C.isEnabled()=%d\r\n", M5.Ex_I2C.isEnabled());
  Serial.printf("cfg.internal_imu=%d\r\n", cfg.internal_imu);
  Serial.printf("cfg.external_imu=%d\r\n", cfg.external_imu);
  Serial.printf("AtomS3.In_I2C.getSCL()/getSDA() = %d/%d\r\n", AtomS3.In_I2C.getSCL(), AtomS3.In_I2C.getSDA());
  for (int i = 0; i < 10; i++) {
    Serial.println(mygetMotorSpeed(0));
  }
  microbit::init();
  microbit::connect(&mymb, MY_PEER);

  queue = xQueueCreate(10, sizeof(sensor_data));
  xTaskCreatePinnedToCore(task1, "task1", 4096, NULL, 1, NULL, 1);

  prop.STATE = 0;
  prop.DAMAGE = 0;
}

void drawString(int color, char *s)
{
  AtomS3.Display.clear();
  AtomS3.Display.setTextColor(color);
  AtomS3.Display.drawString(s, AtomS3.Display.width() / 2, AtomS3.Display.height() / 2);
}

void loop()
{
  static float ix = 0.0, iy = 0.0, iz = 0.0;
  sensor_data s;
  AtomS3.update();
  auto updated = AtomS3.Imu.update();
  if (updated) {
    auto imu = AtomS3.Imu.getImuData();
    ix = 0.05 * imu.accel.x + (1 - 0.05) * ix;
    iy = 0.05 * imu.accel.y + (1 - 0.05) * iy;
    iz = 0.05 * imu.accel.z + (1 - 0.05) * iz;
  }

  if (interruptFlag) {
    Serial.println("HIT!!::stop");
    drawString(RED, "HIT!!");
    AtomicMotion.setMotorSpeed(0, 0);
    AtomicMotion.setMotorSpeed(1, 0);
    delay(1000);  // stop for 1 seconds.
    Serial.println("HIT!!::release");
    xQueueReset(queue);
    drawString(GREEN, "FIGHT!");
    interruptFlag = false;
  }

  xQueueReceive(queue, &s, portMAX_DELAY);

  Serial.printf("mb: %d %d %d %d\r\n", s.buttons, s.accel[0], s.accel[1], s.accel[2]);
  Serial.printf("im: %0.2f %0.2f %0.2f\r\n", ix, iy, iz);

  int y = map(s.accel[1], -1100, +1100, -110, +110);
  int x = map(s.accel[0], -1100, +1100, -1, +2);

  int x0 = 1 - max(0, x);
  int x1 = 1 - max(0, -x);

  int y0 = -int(y * x0);
  int y1 = -int(y * x1);
  Serial.printf("x=%d y=%d y0=%d y1=%d\r\n", x, y, y0, y1);  

  if (0xF0 & s.buttons) {
    mysetMotorSpeed(0, 80);
    mysetMotorSpeed(1, -80);
  } else if (0x0F & s.buttons) {
    mysetMotorSpeed(0, -80);
    mysetMotorSpeed(1, 80);
  } else if (x == 0) {
    mysetMotorSpeed(0, y0);
    mysetMotorSpeed(1, y1);
  } else if ( x > 0) {
    mysetMotorSpeed(0, 100);
    mysetMotorSpeed(1, 0);
  } else if ( x < 0) {
    mysetMotorSpeed(0, 0);
    mysetMotorSpeed(1, 100);
  }
  Serial.println(mygetMotorSpeed(0));
  Serial.println(mygetMotorSpeed(1));
  delay(10);
}
