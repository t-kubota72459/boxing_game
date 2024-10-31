#include "Property.h"

void Property::dump2(uint8_t* s)
{
    int i = 0;
    s[i++] = static_cast<uint8_t>(IAM);
    s[i++] = static_cast<uint8_t>(STATUS);
    s[i++] = static_cast<uint8_t>(POSTURE);
    s[i++] = static_cast<uint8_t>(DAMAGE);
}
