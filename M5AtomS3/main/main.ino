#include "M5AtomS3.h"
#include "microbit.h"

#define MY_PEER "C2:FD:63:8F:CC:42"

microbit mymb = microbit();

void setup()
{
  auto cfg = M5.config();
  AtomS3.begin(cfg);
  Serial.begin(115200);

  microbit::init();
  microbit::connect(&mymb, MY_PEER);
}

void loop()
{
  AtomS3.update();
  Serial.println(std::to_string(mymb.get_a_button()).c_str());
  Serial.println(std::to_string(mymb.get_b_button()).c_str());
  delay(100);
}
