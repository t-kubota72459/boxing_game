#include "M5AtomS3.h"
#include "microbit.h"
#include "BLEDevice.h"

// Button
static const BLEUUID button("e95d9882-251d-470a-a062-fa1922dfa9a8");
static const BLEUUID a_button("e95dda90-251d-470a-a062-fa1922dfa9a8");
static const BLEUUID b_button("e95dda91-251d-470a-a062-fa1922dfa9a8");

// Accelerometer
static const BLEUUID accelerometer("e95d0753-251d-470a-a062-fa1922dfa9a8");
static const BLEUUID accelerometer_data("e95dca4b-251d-470a-a062-fa1922dfa9a8");
static const BLEUUID accelerometer_period("e95dfb24-251d-470a-a062-fa1922dfa9a8");

// UART service
static const BLEUUID uart("6e400001-b5a3-f393-e0a9-e50e24dcca9e");
static const BLEUUID uart_tx("6e400003-b5a3-f393-e0a9-e50e24dcca9e");

bool microbit::init_done = false;

microbit::microbit() {}
microbit::~microbit() {}

bool microbit::init()
{
  if (init_done == false) {
    BLEDevice::init("");
    init_done = true;
  }
  // Serial.println("#init");
  return init_done;
}

bool microbit::connect(microbit* mb, const char* address)
{
  if (init_done) {

    mb->client = BLEDevice::createClient();
    mb->peer = new BLEAddress(address);
    mb->client->connect(*(mb->peer), BLE_ADDR_TYPE_RANDOM);
    if (mb->client->isConnected() == false) {
      //Serial.println("failed to connect the server...");
      return false;
    }
    //Serial.print("#connect::");
    //Serial.println(address);
    if (mb->client == nullptr) {
      //Serial.println("connect::NULL!!!!");
    }
    return true;
  }
  return false;
}

void microbit::set_accelerometer_period(uint32_t data32)
{
  uint8_t tmp[4];
  tmp[0] = data32;
  tmp[1] = data32 >> 8;
  tmp[2] = data32 >> 16;
  tmp[3] = data32 >> 24;

  client->getService(accelerometer)->getCharacteristic(accelerometer_period)->writeValue(tmp, 4, false);
}

void microbit::register_for_notify(notify_callback notifyCallback)
{
  if (client != nullptr) {
    client->getService(accelerometer)->getCharacteristic(accelerometer_data)->registerForNotify(notifyCallback);
  }
}

void microbit::get_accelerometer(int16_t* accel)
{
  if (client != nullptr) {
    std::string s = this->client->getValue(accelerometer, accelerometer_data);
    if (s.length() == 6) {
      for (size_t i = 0, j = 0;  i < s.length(); i+=2, j++) {
        accel[j] = static_cast<int16_t>((static_cast<uint8_t>(s[i+1]) << 8) | static_cast<uint8_t>(s[i]));
      }
    }
  }
  return;
}

uint8_t microbit::get_a_button()
{
  if (client != nullptr) {
    std::string s = client->getValue(button, a_button);
    if (s.length() >= 1) {
      return (uint8_t)s[0];
    }
  }
  return 0;
}

uint8_t microbit::get_b_button()
{
  if (client != nullptr) {
    std::string s = this->client->getValue(button, b_button);
    if (s.length() >= 1) {
      return (uint8_t)s[0];
    }
  }
  return 0;
}
