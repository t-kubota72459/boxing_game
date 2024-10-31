#ifndef _HIMU_H_
#define _HIMU_H_

#include "Posture.h"

struct HImu
{
    static Posture  getPosture();
    static void     update();
    static void     resetPosture();

    static m5::imu_3d_t normalized;
    static Posture posture;
};
#endif
