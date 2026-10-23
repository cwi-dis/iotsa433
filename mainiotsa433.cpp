//
// iotsa433: a WiFi <-> 433 MHz OOK bridge. Receives from 433 MHz remote controls
// and can forward matching telegrams as HTTP GET requests (a trigger -> URL
// "forwarder" engine), and sends commands to 433 MHz sockets/relays via REST,
// its web page or the /433send trigger.
// See readme.md -- the decoders (HEMA / ELRO Flamingo) are brand-specific and
// meant to be adapted to whatever hardware you happen to have.
//

#include <Esp.h>
#include "iotsa.h"
#include <RCSwitch.h>
#include "iotsa433Sender.h"
#include "iotsa433Receiver.h"

IotsaApplication application("Iotsa 433Mhz Home Automation Remote Control Server");

// One radio library for both: the receiver on GPIO 4, the sender on GPIO 5.
RCSwitch rcswitch;
Iotsa433SenderMod senderMod(application, rcswitch, 5);
Iotsa433ReceiverMod receiverMod(application, rcswitch, 4);

void setup(void){
  application.setup();
  application.lateSetup();
  // For some reason trying to create the callbacks with std::bind doesn't work,
  // and crashes during initialization.
  // Transient status-LED pulses (cwi-dis/iotsa#176/#256) -- decay back to the
  // normal status display on their own, no restore logic needed.
  receiverMod.setStatusCallbacks(
    []{ iotsaStatus.setStatusPulse(0x002000, 0, 0, 250, "433 telegram forwarded"); },
    []{ iotsaStatus.setStatusPulse(0x200000, 0, 0, 1000, "433 forward failed"); }
    );
}

void loop(void){
  application.loop();
}
