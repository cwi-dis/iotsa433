#include <Arduino.h>

#ifdef WITH_HEMA
bool decode433_hema(uint32_t dec, int bitLength, String& dip, String& button, String& onoff);
#endif
#ifdef WITH_ELRO_FLAMINGO
bool decode433_elro(uint32_t dec, int bitLength, String& dip, String& button, String& onoff);
String encode433_elro(String dipswitches, String button, int value);
#endif
bool decode433_tristate(uint32_t dec, int bitLength, String& tristate);