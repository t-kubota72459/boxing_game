#include <M5AtomS3.h>
#include "Motor.h"

#define I2C_ATOMIC_MOTION_ADDR  0x38
#define I2C_ATOMIC_MOTION_FREQ  400000

uint8_t Motor::getMotorSpeed(uint8_t ch)
{
    uint8_t reg = ch | 0x20;
    return M5.In_I2C.readRegister8(I2C_ATOMIC_MOTION_ADDR, reg, I2C_ATOMIC_MOTION_FREQ);
}

uint8_t Motor::setMotorSpeed(uint8_t ch, int8_t speed)
{
    uint8_t reg = ch | 0x20;
    M5.In_I2C.writeRegister8(I2C_ATOMIC_MOTION_ADDR, reg, (uint8_t)speed, I2C_ATOMIC_MOTION_FREQ);
    return 0;
}
