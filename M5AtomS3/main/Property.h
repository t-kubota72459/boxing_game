#ifndef _PROPERTY_H_
#define _PROPERTY_H_

#include "M5AtomS3.h"
#include "Player.h"
#include "Status.h"
#include "Posture.h"

struct Property
{
    Player      IAM;            // PLAYER
    Status      STATUS;         // ACTIVE
    Posture     POSTURE;        // POSTURE
    int8_t      DAMAGE;         // DAMAGE
    void dump2(uint8_t *s);
};
#endif
