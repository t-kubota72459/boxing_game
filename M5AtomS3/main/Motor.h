#ifndef Motor_h
#define Motor_h

struct Motor
{
  static uint8_t getMotorSpeed(uint8_t ch);
  static uint8_t setMotorSpeed(uint8_t ch, int8_t speed);
};
#endif
