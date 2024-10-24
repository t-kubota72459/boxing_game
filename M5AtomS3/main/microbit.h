#ifndef microbit_h
#include "BLEClient.h"
#define microbit_h

struct sensor_data
{
    uint8_t buttons;
    int16_t accel[3];
};

struct microbit
{
    static bool init_done;

    BLEClient *client;
    BLEAddress *peer;

    microbit();
    ~microbit();

    static bool init();
    static bool connect(microbit* mb, const char* address);

    void set_accelerometer_period(uint32_t data32);
    void register_for_notify(notify_callback notifyCallback);

    uint8_t get_a_button();
    uint8_t get_b_button();
    void    get_accelerometer(int16_t* accel);
};
#endif
