#include <Arduino.h>

#ifdef WITH_HEMA
bool decode433_hema(uint32_t dec, int bitLength, String& dip, String& appliance, String& state);
#endif
#ifdef WITH_ELRO_FLAMINGO
bool decode433_elro(uint32_t dec, int bitLength, String& dip, String& appliance, String& state);
String encode433_elro(String group, String appliance, int value);
#endif
bool decode433_tristate(uint32_t dec, int bitLength, String& telegram_tristate);