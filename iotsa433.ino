//
// Boilerplate for configurable web server (probably RESTful) running on ESP8266.
//
// The server always includes the Wifi configuration module. You can enable
// other modules with the preprocessor defines. With the default defines the server
// will allow serving of web pages and other documents, and of uploading those.
//

#include <Esp.h>
#include "iotsa.h"
#include "iotsaWifi.h"
#include "iotsa433Send.h"
#include "iotsa433Receive.h"

// CHANGE: Add application includes and declarations here

#define WITH_OTA    // Enable Over The Air updates from ArduinoIDE. Needs at least 1MB flash.

IotsaApplication application("Iotsa 433Mhz Home Automation Remote Control Server");

IotsaWifiMod wifiMod(application);

Iotsa433SendMod sendMod(application); 
Iotsa433ReceiveMod receiveMod(application); 

#ifdef WITH_OTA
#include "iotsaOta.h"
IotsaOtaMod otaMod(application);
#endif

void setup(void){
  application.setup();
  application.serverSetup();
#ifndef ESP32
  ESP.wdtEnable(WDTO_120MS);
#endif
}
 
void loop(void){
  application.loop();
}

