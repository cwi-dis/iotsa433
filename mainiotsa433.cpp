//
// iotsa433: a WiFi <-> 433 MHz OOK bridge. Receives from 433 MHz remote controls
// and can forward matching telegrams as HTTP GET requests (a trigger -> URL
// "forwarder" engine), and sends commands to 433 MHz sockets/relays via REST.
// See readme.md -- the decoders (HEMA / ELRO Flamingo) are brand-specific and
// meant to be adapted to whatever hardware you happen to have.
//

#include <Esp.h>
#include "iotsa.h"
#include "iotsa433Send.h"
#include "iotsa433Receive.h"

IotsaApplication application("Iotsa 433Mhz Home Automation Remote Control Server");

Iotsa433SendMod sendMod(application);
Iotsa433ReceiveMod receiveMod(application);

void setup(void){
  application.setup();
  application.lateSetup();
  // For some reason trying to create the callbacks with std::bind doesn't work,
  // and crashes during initialization.
  // Transient status-LED pulses (cwi-dis/iotsa#176/#256) -- decay back to the
  // normal status display on their own, no restore logic needed.
  receiveMod.setStatusCallbacks(
    []{ iotsaStatus.setStatusPulse(0x002000, 0, 0, 250, "433 signal received"); },
    []{ iotsaStatus.setStatusPulse(0x200000, 0, 0, 1000, "433 signal not decoded"); }
    );
#ifndef ESP32
  ESP.wdtEnable(WDTO_120MS);
#endif
}

void loop(void){
  application.loop();
}
