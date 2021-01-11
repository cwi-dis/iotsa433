#include <Arduino.h>

bool decode433_hema(uint32_t dec, int bitLength, String& dip, String& button, String& onoff);
bool decode433_elro(uint32_t dec, int bitLength, String& dip, String& button, String& onoff);
bool decode433_tristate(uint32_t dec, int bitLength, String& tristate);