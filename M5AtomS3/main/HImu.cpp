#include "M5AtomS3.h"
#include "microbit.h"
#include "Motor.h"
#include "Property.h"
#include "Hmqtt.h"
#include <cmath>
#include "HImu.h"

m5::imu_3d_t HImu::normalized;
Posture HImu::posture;

Posture HImu::getPosture()
{
  if (abs(HImu::normalized.x) > 0.95) {         // 左右どちらかに寝ている
    HImu::posture = Posture::FALLEN;
  } else if (abs(HImu::normalized.z) > 0.95) {  // 伏せている or 仰向け
    HImu::posture = Posture::FALLEN;
  } else if (HImu::normalized.y > -0.5) {       // 逆さまになっている？？？
    HImu::posture = Posture::FALLEN;;
  } else {
    HImu::posture = Posture::STAND;
  }
  return HImu::posture;
}

void HImu::update()
{
    m5::imu_3d_t accel;
    auto updated = AtomS3.Imu.update();

    if (updated) {
      AtomS3.Imu.getAccel(&(accel.x), &(accel.y), &(accel.z));
      // Serial.printf("ax=%f, ay=%f, az=%f\r\n", accel.x, accel.y, accel.z);
      HImu::normalized.x = 0.3 * accel.x + (1 - 0.3) * HImu::normalized.x;
      HImu::normalized.y = 0.3 * accel.y + (1 - 0.3) * HImu::normalized.y;
      HImu::normalized.z = 0.3 * accel.z + (1 - 0.3) * HImu::normalized.z;
   } 
}

void HImu::resetPosture()
{
    HImu::posture = Posture::STAND;

    HImu::normalized.x = 0.0;
    HImu::normalized.y = 0.0;
    HImu::normalized.z = 0.0;
}
