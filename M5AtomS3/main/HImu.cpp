#include "M5AtomS3.h"
#include "microbit.h"
#include "Motor.h"
#include "Property.h"
#include "Hmqtt.h"
#include <cmath>
#include "HImu.h"

m5::imu_3d_t HImu::normalized;
Posture HImu::posture;

static int fallenCount = 0;
static const int fallenThreshold = 3;

Posture HImu::getPosture()
{
    char s[256];
    if ( (abs(HImu::normalized.x) > 0.8) || (abs(HImu::normalized.z) > 0.8) || (HImu::normalized.y > 0.8) ) {
        fallenCount++;
        if (fallenCount >= fallenThreshold) {
            HImu::posture = Posture::FALLEN;
            sprintf(s, "Fallen X=%.2f Y=%.2f Z=%.2f", HImu::normalized.x, HImu::normalized.y, HImu::normalized.z);
            AtomS3.Display.clear();
            AtomS3.Display.setTextSize(1);
            AtomS3.Display.drawString(s, AtomS3.Display.width() / 2, int(AtomS3.Display.height() * 3.0/4.0));
        }
    } else {
        fallenCount = 0;
        HImu::posture = Posture::STAND;
    }
    return HImu::posture;
}

void HImu::update()
{
    m5::imu_3d_t accel;
    
    if ( AtomS3.Imu.update() ) {
      AtomS3.Imu.getAccel(&(accel.x), &(accel.y), &(accel.z));
      // Serial.printf("ax=%f, ay=%f, az=%f\r\n", accel.x, accel.y, accel.z);
      HImu::normalized.x = 0.2 * accel.x + 0.8 * HImu::normalized.x;
      HImu::normalized.y = 0.2 * accel.y + 0.8 * HImu::normalized.y;
      HImu::normalized.z = 0.2 * accel.z + 0.8 * HImu::normalized.z;
   } 
}

void HImu::resetPosture()
{
    HImu::posture = Posture::STAND;
    HImu::normalized.x = 0.0;
    HImu::normalized.y = 0.0;
    HImu::normalized.z = 0.0;
}
