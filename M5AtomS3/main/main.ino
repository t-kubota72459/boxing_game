#include "M5AtomS3.h"
#include "microbit.h"
#include "Motor.h"
#include "Property.h"
#include "Hmqtt.h"
#include "HImu.h"

#define BOXIER1             "F6:CC:2F:23:3B:6C"
#define BOXIER2             "D5:D7:F2:2E:47:FB"
#define BOXIER3             "D8:10:15:D8:17:95"

// つなげる Micro:bit の MAC
#define MY_MICROBIT         BOXIER1

// プレイヤー
#define SELF                Player::PLAYER1

const char *Status_string_table[] {
    "none",
    "IDL",
    "FIT",
    "WIN",
    "DRW",
    "LOS",
    "BUS",
    "RDY"
};

volatile bool interruptFlag1 = false;
volatile bool interruptFlag = false;

Property prop;
static uint8_t msg[16];

const unsigned long debounceTime = 25;              // Set debounce time in milliseconds
volatile unsigned long lastInterruptTime = 0;
void IRAM_ATTR handleInterrupt()
{
    unsigned long currentTime = millis();
    if ((currentTime - lastInterruptTime) > debounceTime) {
        if (! interruptFlag1) {
            interruptFlag1 = true;
        } else {
            interruptFlag = true;
        }
        lastInterruptTime = currentTime;
    }        
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

    prop.IAM = SELF;
    prop.STATUS = Status::BUSY;
    
    AtomS3.Display.setRotation(2);
    AtomS3.Display.setTextColor(GREEN);
    AtomS3.Display.setTextDatum(middle_center);
    AtomS3.Display.setTextSize(4);
    
    // Initialize motors
    Serial.printf("Motor: init.\r\n");
    Motor::initMotors();
    while (Motor::getMotorSpeed(0) || Motor::getMotorSpeed(1)) {
        Serial.printf("Motor: ch0=%d, ch1=%d\r\n", Motor::getMotorSpeed(0), Motor::getMotorSpeed(1));
    }
    Serial.printf("Motor: done.\r\n");
    
    // Initialize imu (posture)
    Serial.printf("Imu(Posture): init.\r\n");
    HImu::resetPosture();
    while ( (prop.POSTURE = HImu::getPosture()) == Posture::FALLEN ) {
        HImu::update();
        drawString(RED, "^");
        Serial.printf("Imu: posture=%d\r\n", static_cast<int>(HImu::getPosture()));
        delay(100);
        
    }
    Serial.printf("Imu(Posture): done.\r\n");

    // set interrupt
    Serial.printf("Interrupt: init.\r\n");
    pinMode(1, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(1), handleInterrupt, FALLING);
    prop.DAMAGE = 0;
    Serial.printf("Interrupt: done.\r\n");
    delay(10);

    Serial.printf("Microbit: init %s.\r\n", MY_MICROBIT);
    drawString(RED, "MB");
    microbit::init();
    microbit::connect(&mymb, MY_MICROBIT);
    Serial.printf("Microbit: done.");
    drawString(RED, "MB.");
    delay(10);

    Serial.printf("Acquistion thread: init.\r\n");
    queue = xQueueCreate(10, sizeof(sensor_data));
    xTaskCreatePinnedToCore(task1, "task1", 4096, NULL, 1, NULL, 1);
    Serial.printf("Acquistion thread: done.\r\n");
    delay(10);

    Serial.printf("MQTT: init.\r\n");
    drawString(RED, "WF");
    Hmqtt::init();
    delay(10);
    Hmqtt::update();
    delay(10);
    drawString(RED, "WF.");
    Serial.printf("MQTT: done.\r\n");
    
    prop.STATUS = Status::IDLE;

    Serial.print("MQTT: send my status....");
    prop.dump2(msg);
    Hmqtt::client.publish("HTC_BOXING/STATUS", msg, (unsigned int)4, false);
    Serial.println("done.");

    Serial.printf("prop.POSTURE=%d\r\n", static_cast<int8_t>(prop.POSTURE));
    
    AtomS3.Display.setRotation(2);    // 180 deg
    AtomS3.Display.setTextColor(GREEN);
    AtomS3.Display.setTextDatum(middle_center);
    AtomS3.Display.setTextSize(4);
    
    AtomS3.Display.clear();
    
    drawString(GREEN, Status_string_table[static_cast<int8_t>(prop.STATUS)]);
}

void drawString(int color, const char *s)
{
    AtomS3.Display.clear();
    AtomS3.Display.setTextColor(color);
    AtomS3.Display.drawString(s, AtomS3.Display.width() / 2, AtomS3.Display.height() / 2);
}

static Status prev_stat = Status::BUSY;
static bool fallen_send_flag = false;


void loop()
{
    sensor_data s;
    
    int x, x0, x1, y, y0, y1, i;

    AtomS3.update();
    Hmqtt::update();
    HImu::update();

    xQueueReceive(queue, &s, portMAX_DELAY);

    switch (prop.STATUS) {
    case Status::FIGHT:
        // 鼻ヒット！
        if ( interruptFlag ) {
            char bikkuri[16];
            prop.DAMAGE++;
            prop.dump2(msg);
            for (i = 0; i < prop.DAMAGE; i++) {
                bikkuri[i] = '!';
            }
            bikkuri[i] = '\0';
            drawString(RED, bikkuri);
                
            Hmqtt::client.publish("HTC_BOXING/STATUS", msg, (unsigned int)4, false);
            Motor::initMotors();
            delay(1000);  // stop for 1 seconds.
            xQueueReset(queue);
            interruptFlag1 = false;
            interruptFlag = false;
            drawString(GREEN, Status_string_table[static_cast<int8_t>(prop.STATUS)]);
        }
        // 倒れた！
        if (! fallen_send_flag && ((prop.POSTURE = HImu::getPosture()) == Posture::FALLEN )) {
            prop.dump2(msg);
            Hmqtt::client.publish("HTC_BOXING/STATUS", msg, (unsigned int)4, false);
            fallen_send_flag = true;
            // 送るのは１回のみ
            // リセットするとまた送れる
        }

        // Serial.printf("microbit: buttons=%d X=%d Y=%d Z=%d\r\n", s.buttons, s.accel[0], s.accel[1], s.accel[2]);

        y = map(s.accel[1], -1100, +1100, -90, +91);  // range from -90 to +91
        x = map(s.accel[0], -1100, +1100, -1, +2);      // range from -1 to +1

        x0 = 1 - max(0, x);
        x1 = 1 - max(0, -x);
        y0 = -int(y * x0);
        y1 = -int(y * x1);

        // Serial.printf("x=%d y=%d y0=%d y1=%d\r\n", x, y, y0, y1);  

        if (0xF0 & s.buttons) {               // turn right
            Motor::setMotorSpeed(0, 90);
            Motor::setMotorSpeed(1, -(-90));
        } else if (0x0F & s.buttons) {        // turn left
            Motor::setMotorSpeed(0, -90);
            Motor::setMotorSpeed(1, -90);
        } else if (x == 0) {                  // forward or back 
            Motor::setMotorSpeed(0, y0);
            Motor::setMotorSpeed(1, -y1);
        } else if ( x > 0) {                  // right punch only
            Motor::setMotorSpeed(0, 90);
            Motor::setMotorSpeed(1, 0);
        } else if ( x < 0) {                  // left punch only
            Motor::setMotorSpeed(0, 0);
            Motor::setMotorSpeed(1, -90);
        }
        // Serial.printf("motor: ch0=%d, ch1=%d\r\n", Motor::getMotorSpeed(0), Motor::getMotorSpeed(1));
        break;

    case Status::WIN:
    case Status::LOSE:
    case Status::DRAW:
        Motor::setMotorSpeed(0, 0);
        Motor::setMotorSpeed(1, 0);
        break;

    case Status::READY:
        Motor::setMotorSpeed(0, 0);
        Motor::setMotorSpeed(1, 0);

        prop.STATUS = Status::IDLE;        
        prop.POSTURE = HImu::getPosture();
        prop.DAMAGE = 0;
        prop.dump2(msg);
        fallen_send_flag = false;
        Serial.printf("READY done, STATUS to IDLE.\r\n");
        Hmqtt::client.publish("HTC_BOXING/STATUS", msg, (unsigned int)4, false);
        break;

    case Status::IDLE:
        Motor::setMotorSpeed(0, 0);
        Motor::setMotorSpeed(1, 0);
        break;
    
    default:
        Serial.println("Error!!!\r\n");
        Motor::setMotorSpeed(0, 0);
        Motor::setMotorSpeed(1, 0);
        // should not come here
    }

    if (prev_stat != prop.STATUS) {
      drawString(GREEN, Status_string_table[static_cast<int8_t>(prop.STATUS)]);
    }

    prev_stat = prop.STATUS;
    delay(10);
}
