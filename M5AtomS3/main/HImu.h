#ifndef _HImu_H_
#define _HImu_H_

struct HImu
{
    static uint8_t  getPosture();
    static void     resetPosture();

    static m5::imu_3d_t normalized;
    static uint8_t posture;
};
#endif
