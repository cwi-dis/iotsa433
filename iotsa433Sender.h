#ifndef _IOTSA433SENDER_H_
#define _IOTSA433SENDER_H_
#include "iotsa.h"
#include "iotsaApi.h"
#include <RCSwitch.h>

//
// The 433 MHz sender: sends a command to a socket or relay. Three ways in,
// for the same command:
// - the web page /433sender, for people;
// - PUT or POST /api/433sender;
// - the trigger /433send?group=01011&appliance=A&state=on, a plain URL for
//   curl or another device. A forwarder of another iotsa433 with "parameters"
//   set calls it like this, which makes a relay.
//

// One command, as read from a request: a tristate telegram, a binary
// telegram, or a switch command (brand, group, appliance, state).
struct Iotsa433Command {
  String tristate;      // 0, 1 or F per position
  String binary;        // 0 or 1 per bit
  String brand;         // optional: HEMA (the default), ELRO
  String group;         // the DIP switches, like 01011
  String appliance;     // A-E, 0-5, or a bit pattern like 01000
  String state;         // on/1/true or off/0/false
  int protocol = 0;     // rc-switch protocol, 0: the brand's default (1, ELRO 13)
  int pulsewidth = 0;   // microseconds, 0: the protocol's default
};

class Iotsa433SenderMod : public IotsaModule {
public:
  Iotsa433SenderMod(IotsaApplication &_app, RCSwitch &_rcswitch, int _pin)
  : IotsaModule(_app),
    rcswitch(_rcswitch),
    pin(_pin)
  {}
  void setup() override;
  void lateSetup() override;
  void loop() override;
  String info() override;
protected:
  void getHandler(IotsaApiContext& ctx) override;
  void putHandler(IotsaApiContext& ctx) override;
  void postHandler(IotsaApiContext& ctx) override;
#ifdef IOTSA_WITH_WEB
  String webTitle() override { return "433 MHz sender"; }
  void formHandler_args(IotsaWebContext& ctx) override;
  void formHandler_fields(IotsaWebContext& ctx) override;
  void sendTrigger(IotsaWebContext& ctx);
#endif
  // Send cmd, or say why not.
  bool _send(const Iotsa433Command& cmd, String& error);
  bool _sendSwitch(const Iotsa433Command& cmd, bool on, String& error);
  void _setProtocol(int protocol, int pulsewidth);
  RCSwitch &rcswitch;
  int pin;
};

#endif
