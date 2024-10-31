#ifndef _MOTOR_H_
#define _MOTOR_H_

struct Motor
{
  static uint8_t    getMotorSpeed(uint8_t ch);
  static uint8_t    setMotorSpeed(uint8_t ch, int8_t speed);
  static void       initMotors();
};
#endif
