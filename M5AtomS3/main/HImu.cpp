#include "M5AtomS3.h"
#include "microbit.h"
#include "Motor.h"
#include "Property.h"
#include "Hmqtt.h"
#include <cmath>
#include "HImu.h"

m5::imu_3d_t HImu::normalized;
uint8_t HImu::posture;

uint8_t HImu::getPosture()
{
    m5::imu_3d_t accel;
    auto updated = AtomS3.Imu.update();
    
    if (updated) {
        AtomS3.Imu.getAccel(&(accel.x), &(accel.y), &(accel.z));
        HImu::normalized.x = 0.1 * accel.x + (1 - 0.1) * HImu::normalized.x;
        HImu::normalized.y = 0.1 * accel.y + (1 - 0.1) * HImu::normalized.y;
        HImu::normalized.z = 0.1 * accel.z + (1 - 0.1) * HImu::normalized.z;
      if (abs(HImu::normalized.x) > 0.75) {
        HImu::posture = 1;
      } else if (abs(HImu::normalized.z) > 0.75) {
        HImu::posture = 1;
      } else if (HImu::normalized.y > -0.5) {
        HImu::posture = 1;
      } else {
        HImu::posture = 0;
      }
    }
    return HImu::posture;
}

void HImu::resetPosture()
{
    HImu::normalized.x = 0.0;
    HImu::normalized.y = 0.0;
    HImu::normalized.z = 0.0;
    HImu::posture = 0;
}
