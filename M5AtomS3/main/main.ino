#include "M5AtomS3.h"
#include "microbit.h"
#include "Motor.h"
#include "Property.h"
#include "Hmqtt.h"
#include "HImu.h"

#define MY_MICROBIT "C2:FD:63:8F:CC:42"
// #define MY_PEER "D9:1F:4C:D4:A8:35"

volatile bool interruptFlag = false;

Property prop;
static uint8_t msg[16];

void IRAM_ATTR handleInterrupt()
{
    interruptFlag = true;
}

microbit mymb = microbit();
QueueHandle_t queue;

void task1(void* pvParameters)
{
    sensor_data s;

    while ( 1 ) {
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
    Serial.begin(115200);

    prop.IAM = Player::PLAYER1;
    prop.STATUS = Status::BUSY;

    // Initialize motors
    Motor::initMotors();
    while (Motor::getMotorSpeed(0) || Motor::getMotorSpeed(1)) {
        Serial.printf("Motor: ch0=%d, ch1=%d\r\n", Motor::getMotorSpeed(0), Motor::getMotorSpeed(1));
    }
    Serial.printf("Motor: init.\r\n");
    
    // Initialize imu (posture)
    HImu::resetPosture();
    while ( (prop.POSTURE = HImu::getPosture()) == Posture::FALLEN ) {
        HImu::update();
        Serial.printf("Imu: posture=%d\r\n", static_cast<int>(HImu::getPosture()));
        delay(10);
    }
    Serial.printf("Imu: init.\r\n");

    // set interrupt
    pinMode(1, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(1), handleInterrupt, FALLING);
    prop.DAMAGE = 0;

    Serial.printf("Interrupt init.\r\n");
    delay(10);

    microbit::init();
    microbit::connect(&mymb, MY_MICROBIT);
    Serial.printf("Microbit: init. %s\r\n", MY_MICROBIT);
    delay(10);

    queue = xQueueCreate(10, sizeof(sensor_data));
    xTaskCreatePinnedToCore(task1, "task1", 4096, NULL, 1, NULL, 1);
    Serial.printf("Acquistion thread: init.\r\n");
    delay(10);

    Hmqtt::init();
    delay(10);
    Serial.printf("MQTT: init\r\n");
    Hmqtt::update();
    delay(10);
    Serial.printf("MQTT: update\r\n");
    
    prop.STATUS = Status::IDLE;

    Serial.print("mqtt send....");
    prop.dump2(msg);
    Hmqtt::client.publish("HTC_BOXING/STATUS", msg, (unsigned int)4, false);
    Serial.println("done.");
    Serial.printf("prop.POSTURE=%d\r\n", static_cast<int8_t>(prop.POSTURE));
    AtomS3.Display.setRotation(2);    // 180 deg
    AtomS3.Display.setTextColor(GREEN);
    AtomS3.Display.setTextDatum(middle_center);
    AtomS3.Display.setTextSize(4);
    
    AtomS3.Display.clear();
    AtomS3.Display.drawString("OK", AtomS3.Display.width() / 2, AtomS3.Display.height() / 2);
}

void drawString(int color, const char *s)
{
    AtomS3.Display.clear();
    AtomS3.Display.setTextColor(color);
    AtomS3.Display.drawString(s, AtomS3.Display.width() / 2, AtomS3.Display.height() / 2);
}

static Status prev_stat = Status::BUSY;

void loop()
{
    sensor_data s;
    int x, x0, x1, y, y0, y1;

    AtomS3.update();
    Hmqtt::update();
    HImu::update();

    xQueueReceive(queue, &s, portMAX_DELAY);

    switch (prop.STATUS) {
    case Status::FIGHT:
        // 鼻ヒット！
        if ( interruptFlag ) {
            drawString(RED, "!!");
            prop.DAMAGE++;
            prop.dump2(msg);
            Hmqtt::client.publish("HTC_BOXING/STATUS", msg, (unsigned int)4, false);
            Motor::initMotors();
            delay(1000);  // stop for 1 seconds.
            xQueueReset(queue);
            drawString(GREEN, "F");
            interruptFlag = false;
        }
        // 倒れた！
        if ( (prop.POSTURE = HImu::getPosture()) == Posture::FALLEN) {
            prop.dump2(msg);
            Hmqtt::client.publish("HTC_BOXING/STATUS", msg, (unsigned int)4, false);
        }

        Serial.printf("microbit: buttons=%d X=%d Y=%d Z=%d\r\n", s.buttons, s.accel[0], s.accel[1], s.accel[2]);

        y = map(s.accel[1], -1100, +1100, -100, +101);  // range from -110 to +110
        x = map(s.accel[0], -1100, +1100, -1, +2);      // range from -1 to +1

        x0 = 1 - max(0, x);
        x1 = 1 - max(0, -x);
        y0 = -int(y * x0);
        y1 = -int(y * x1);

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
        break;

    case Status::WIN:
    case Status::LOSE:
    case Status::DRAW:
        Motor::setMotorSpeed(0, 0);
        Motor::setMotorSpeed(0, 0);
        break;

    case Status::READY:
        Motor::setMotorSpeed(0, 0);
        Motor::setMotorSpeed(0, 0);

        prop.STATUS = Status::IDLE;        
        prop.POSTURE = Posture::STAND;
        prop.DAMAGE = 0;
        prop.dump2(msg);
        Serial.printf("RESET done.\r\n");
        Hmqtt::client.publish("HTC_BOXING/STATUS", msg, (unsigned int)4, false);
        break;

    case Status::IDLE:
        // After swith-on / initialized
        break;
    
    default:
        Serial.println("Error!!!\r\n");
        Motor::setMotorSpeed(0, 0);
        Motor::setMotorSpeed(0, 0);
        // should not come here
    }

    if (prev_stat != prop.STATUS) {
      drawString(GREEN, std::to_string(static_cast<int8_t>(prop.STATUS)).c_str());
    }

    prev_stat = prop.STATUS;
    delay(10);
}
