#include "M5AtomS3.h"
#include "microbit.h"
#include "Motor.h"
#include "Property.h"
#include "Hmqtt.h"
#include "HImu.h"

#define MY_PEER "C2:FD:63:8F:CC:42"
// #define MY_PEER "D9:1F:4C:D4:A8:35"

volatile bool interruptFlag = false;

Property prop;

void IRAM_ATTR handleInterrupt()
{
    interruptFlag = true;
    
}

microbit mymb = microbit();
QueueHandle_t queue;

void task1(void* pvParameters)
{
    sensor_data s;

    while (1) {
        s.buttons = (uint16_t)(((uint8_t)mymb.get_a_button()) << 4 | ((uint8_t)mymb.get_b_button()));
        mymb.get_accelerometer(s.accel);
        xQueueSend(queue, &s, 1);
        delay(20);
    }
}

void setup()
{
    auto cfg = M5.config();
    AtomS3.begin(cfg);

    prop.MODE = 1;

    // Initialize motors
    Motor::initMotors();
    while (Motor::getMotorSpeed(0) || Motor::getMotorSpeed(1)) {
        Serial.printf("Motor: ch0=%d, ch1=%d\r\n", Motor::getMotorSpeed(0), Motor::getMotorSpeed(1));
    }

    // Initialize imu (posture)
    HImu::resetPosture();
    while ( Imu::getPosture() ) {
        Serial.printf("Imu: posture=%d\r\n", Imu::getPosture());
    }

    // set interrupt
    pinMode(1, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(1), handleInterrupt, FALLING);
    prop.DAMAGE = 0;

    AtomS3.Display.setRotation(2);    // 180 deg
    AtomS3.Display.setTextColor(GREEN);
    AtomS3.Display.setTextDatum(middle_center);
    AtomS3.Display.setTextSize(2);

    AtomS3.Display.clear();
    AtomS3.Display.drawString("OK", AtomS3.Display.width() / 2, AtomS3.Display.height() / 2);

    Serial.begin(115200);

    microbit::init();
    microbit::connect(&mymb, MY_PEER);

    queue = xQueueCreate(10, sizeof(sensor_data));
    xTaskCreatePinnedToCore(task1, "task1", 4096, NULL, 1, NULL, 1);
    
    Hmqtt::init();

    prop.MODE = 0;
}

void drawString(int color, char *s)
{
    AtomS3.Display.clear();
    AtomS3.Display.setTextColor(color);
    AtomS3.Display.drawString(s, AtomS3.Display.width() / 2, AtomS3.Display.height() / 2);
}

void loop()
{
    sensor_data s;

    AtomS3.update();
    Hmqtt::update();

    Serial.printf("Mqtt::state=%d\r\n", Hmqtt::client.state());
    
    // 鼻ヒット！
    if ( interruptFlag ) {
        drawString(RED, "HIT!!");
        prop.DAMAGE++;
        
        Hmqtt::client.publish("HTC_BOXING", "JOUTAI");
        
        Motor::initMotors();
        delay(1000);  // stop for 1 seconds.
        xQueueReset(queue);
        drawString(GREEN, "FIGHT!");
        interruptFlag = false;
    }
    
    // 倒れた！
    if ( getPosture() ) {
        Hmqtt::client.publish("HTC_BOXING", "JOUTAI");
    }

    xQueueReceive(queue, &s, portMAX_DELAY);

    Serial.printf("microbit: buttons=%d X=%d Y=%d Z=%d\r\n", s.buttons, s.accel[0], s.accel[1], s.accel[2]);
    Serial.printf("imu: X=%0.2f Y=%0.2f Z=%0.2f\r\n", ix, iy, iz);

    int y = map(s.accel[1], -1100, +1100, -110, +111);  // range from -110 to +110
    int x = map(s.accel[0], -1100, +1100, -1, +2);      // range from -1 to +1

    int x0 = 1 - max(0, x);
    int x1 = 1 - max(0, -x);

    int y0 = -int(y * x0);
    int y1 = -int(y * x1);

    Serial.printf("x=%d y=%d y0=%d y1=%d\r\n", x, y, y0, y1);  

    if (0xF0 & s.buttons) {               // turn right
        Motor::setMotorSpeed(0, 80);
        Motor::setMotorSpeed(1, -(-80));
    } else if (0x0F & s.buttons) {        // turn left
        Motor::setMotorSpeed(0, -80);
        Motor::setMotorSpeed(1, -80);
    } else if (x == 0) {                  // forward or back 
        Motor::setMotorSpeed(0, y0);
        Motor::setMotorSpeed(1, -y1);
    } else if ( x > 0) {                  // right punch only
        Motor::setMotorSpeed(0, 100);
        Motor::setMotorSpeed(1, 0);
    } else if ( x < 0) {                  // left punch only
        Motor::setMotorSpeed(0, 0);
        Motor::setMotorSpeed(1, -100);
    }
    Serial.printf("motor: ch0=%d, ch1=%d\r\n", Motor::getMotorSpeed(0), Motor::getMotorSpeed(1));
    delay(10);
}
