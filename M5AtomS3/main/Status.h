#ifndef _STATUS_H_
#define _STATUS_H_

#include "M5AtomS3.h"

enum class Status : int8_t
{
    IDLE = 1,
    FIGHT = 2,
    WIN = 3,
    DRAW = 4,
    LOSE = 5,
    BUSY = 6,
    READY = 7,
};

#endif
