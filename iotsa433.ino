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
#define WITH_LED    // Enable status led
#define NEOPIXEL_PIN 15

IotsaApplication application("Iotsa 433Mhz Home Automation Remote Control Server");

IotsaWifiMod wifiMod(application);

Iotsa433SendMod sendMod(application);
Iotsa433ReceiveMod receiveMod(application);

#ifdef WITH_OTA
#include "iotsaOta.h"
IotsaOtaMod otaMod(application);
#endif

#ifdef WITH_LED
#include "iotsaLed.h"
IotsaLedMod ledMod(application, NEOPIXEL_PIN);
Iotsa433ReceiveCallback showStatusOk;
Iotsa433ReceiveCallback showStatusNotOk;
#endif


void setup(void){
  application.setup();
  application.serverSetup();
#if 0
  showStatusOk = std::bind(&IotsaLedMod::set, ledMod, 0x002000, 250, 0, 1);
  showStatusNotOk = std::bind(&IotsaLedMod::set, ledMod, 0x200000, 1000, 0, 1);
#endif
  // xxxjack receiveMod.setStatusCallbacks(showStatusOk, showStatusNotOk);
  receiveMod.setStatusCallbacks(
    []{ ledMod.set(0x002000, 250, 0, 1); }, 
    []{ ledMod.set(0x200000, 1000, 0, 1); }
    );
#ifndef ESP32
  ESP.wdtEnable(WDTO_120MS);
#endif
}
 
void loop(void){
  application.loop();
}

