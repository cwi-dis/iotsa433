#include "iotsa.h"
#include "iotsa433Send.h"
#include "iotsaConfigFile.h"
#include <RCSwitch.h>
#include "decode433.h"

extern RCSwitch switch433; // Note: shared variable with Iotsa433Receive
int switch433_pin_send = 5;

bool Iotsa433SendMod::_send_binary(int protocol, int bittime, String binary) {
  if (protocol < 0) protocol = 1; // Default protocol
  if (bittime > 0) {
    switch433.setProtocol(protocol, bittime); // Set protocol, override bittime
  } else {
    switch433.setProtocol(protocol); // Set protocol, use default bittime
  }
  switch433.send(binary.c_str());
  return true;
}

bool Iotsa433SendMod::_send_tristate(int protocol, int bittime, String tristate) {
  if (protocol < 0) protocol = 1; // Default protocol
  if (bittime > 0) {
    switch433.setProtocol(protocol, bittime); // Set protocol, override bittime
  } else {
    switch433.setProtocol(protocol); // Set protocol, use default bittime
  }
  switch433.sendTriState(tristate.c_str());
  return true;
}

bool Iotsa433SendMod::_send_brand(int protocol, int bittime, String brand, String dipswitches, String button, bool onoff) {
  if (protocol < 0) protocol = 1; // Default protocol
  if (bittime > 0) {
    switch433.setProtocol(protocol, bittime); // Set protocol, override bittime
  } else {
    switch433.setProtocol(protocol); // Set protocol, use default bittime
  }
#ifdef WITH_HEMA
  if (brand == "HEMA") brand = ""; // default behaviour
#endif
#ifdef WITH_ELRO_FLAMINGO
  if (brand == "ELRO") {
    String binary = encode433_elro(dipswitches, button, (int)onoff);
    switch433.send(binary.c_str());
    return true;
  }
#endif
  if (brand == "") {
    // Default treatment. We support ABCDE or 12345, otherwise button is a bitstring.
    int buttonNum = -1;
    if (button == "A" || button == "B" || button == "C" || button == "D" || button == "E") {
      // Hema-style device
      buttonNum = button[0] - 'A';
    }
    if (button == "0" || button == "1" || button == "2" || button == "3" || button == "4" || button == "5") {
      // Numbered buttons
      buttonNum = button[0] - '0';
    }
    if (buttonNum >= 0) {
      if (onoff) {
        switch433.switchOn(dipswitches.c_str(), buttonNum);
      } else {
        switch433.switchOff(dipswitches.c_str(), buttonNum);
      }
    } else {
      // Presume button is a mask-style string
      if (onoff) {
        switch433.switchOn(dipswitches.c_str(), button.c_str());
      } else {
        switch433.switchOff(dipswitches.c_str(), button.c_str());
      }
    }
    return true;
  }
  // Unsupported brand.
  return false;
}

#ifdef IOTSA_WITH_WEB
void
Iotsa433SendMod::handler() {
  int protocol = -1;
  int bitTime = -1;
  if (server->hasArg("protocol")) {
    protocol = server->arg("protocol").toInt();
  }
  if (server->hasArg("bitTime")) {
    bitTime = server->arg("bitTime").toInt();
  }

  if (server->hasArg("tristate")) {
    // Send tristate command
    String tristate = server->arg("tristate");
    if (_send_tristate(protocol, bitTime, tristate)) {
      server->send(200, "text/plain", "OK");
    } else {
      server->send(400, "text/plain", "Bad tristate value");
    }
    return;
  }

  if (server->hasArg("binary")) {
    // Send binary command
    String binary = server->arg("binary");
    if (_send_binary(protocol, bitTime, binary)) {
      server->send(200, "text/plain", "OK");
    } else {
      server->send(400, "text/plain", "Bad binary value");
    }
    return;
  }

  String brand;
  if (server->hasArg("brand")) brand = server->arg("brand");
  int nArgs = 0;

  if (server->hasArg("onoff")) nArgs++;
  if (server->hasArg("dipswitches")) nArgs++;
  if (server->hasArg("button")) nArgs++;
  if (nArgs == 3) {
    // We have all arguments. Send command.
    String dipswitches = server->arg("dipswitches");
    String button = server->arg("button");
    String onoff = server->arg("onoff");
    bool on = (onoff == "on" || onoff == "1");
    if (!on && ! (onoff == "off" || onoff == "0" || onoff == "")) {
      server->send(400, "text/plain", "Bad onoff value");
      return;
    }

    if (_send_brand(protocol, bitTime, brand, dipswitches, button, onoff))  {
      server->send(200, "text/plain", "OK");
    } else {
      server->send(400, "text/plain", "Bad command");
    }
    return;
  } else
  if (nArgs != 0) {
    // Some but not all arguments. Error.
    server->send(400, "text/plain", "Must have all of onoff, dipswitches, button");
    return;
  }
  //
  // No command found. Present form for user input.
  //
  String message = "<html><head><title>433MHz sender module</title></head><body><h1>433 MHz sender module</h1>";

  message += "<h2>Send Command</h2><form method='get'><table>";
  message += "<tr><td>Brand:</td><td><input name='brand'></td>";
  message += "<td><i>(example: HEMA, ELRO, empty)</i></td></tr>";
  message += "<tr><td>Dipswitches:</td><td><input name='dipswitches'></td>";
  message += "<td><i>(example: 01011, how DIP switches on device are set)</i></td></tr>";
  message += "<tr><td>Button:</td><td><input name='button'></td>";
  message += "<td><i>(example: A, label on remote control button, or 01000, binary bitpattern)</i></td></tr>";
  message += "<tr><td>On/Off:</td><td><input name='onoff'></td>";
  message += "<td><i>(example: on or off)</i><br></td></tr>";
  message += "<tr><td>Protocol:</td><td><input name='protocol' value='1'></td>";
  message += "<td><i>(433 bit protocol, usually 1, see receiver dump)</i><br></td></tr>";
  message += "<tr><td>BitTime:</td><td><input name='bitTime' value='300'></td>";
  message += "<td><i>(pulse duration in microseconds, see receiver dump)</i><br></td></tr>";
  message += "<tr><td></td><td><input type='submit' value='Send Switch Command'></td></tr></table></form>";

  message += "<h2>Send Geek-style command</h2><form method='get'>";
  message += "TriState command: <input name='tristate'>";
  message += "<i>(example: 00000FFF0FF0)</i><br>";
  message += "Protocol: <input name='protocol' value='1'><br>";
  message += "BitTime: <input name='bitTime' value='300'><br>";
  message += "<input type='submit' value='Send TriState'></form>";

  message += "<h2>Send binary command</h2><form method='get'>";
  message += "Binary command: <input name='binary'>";
  message += "<i>(example: 010100000000000000010100)</i><br>";
  message += "Protocol: <input name='protocol' value='1'><br>";
  message += "BitTime: <input name='bitTime' value='300'><br>";
  message += "<input type='submit' value='Send binary'></form>";

  server->send(200, "text/html", message);
}

String Iotsa433SendMod::info() {
  String message = "<p>Built with 433 MHz sender module. See <a href=\"/433send\">/433send</a> to change the devices.</p>";
  return message;
}
#endif // IOTSA_WITH_WEB

void Iotsa433SendMod::setup() {
  configLoad();
  switch433.enableTransmit(switch433_pin_send);
  //
  // Note: no support for setProtocol(), setPulseLength() and setRepeatTransmit()
  // but they should be relatively easy to add.
  //
}

#ifdef IOTSA_WITH_API
bool Iotsa433SendMod::getHandler(const char *path, JsonObject& reply) {
#if 0
  reply["argument"] = argument;
#endif
  return true;
}

bool Iotsa433SendMod::putHandler(const char *path, const JsonVariant& request, JsonObject& reply) {
  bool anyChanged = false;
#if 0
  JsonObject reqObj = request.as<JsonObject>();
  if (reqObj.containsKey("argument")) {
    argument = reqObj["argument"].as<String>();
    anyChanged = true;
  }
  if (anyChanged) configSave();
#endif
  return anyChanged;
}
#endif // IOTSA_WITH_API

void Iotsa433SendMod::serverSetup() {
#ifdef IOTSA_WITH_WEB
  server->on("/433send", std::bind(&Iotsa433SendMod::handler, this));
#endif
#ifdef IOTSA_WITH_API
  api.setup("/api/433send", true, true);
  name = "433send";
#endif
}

void Iotsa433SendMod::configLoad() {
  IotsaConfigFileLoad cf("/config/433send.cfg");
#if 0
  cf.get("argument", argument, "");
#endif
 
}

void Iotsa433SendMod::configSave() {
  IotsaConfigFileSave cf("/config/433send.cfg");
#if 0
  cf.put("argument", argument);
#endif
}

void Iotsa433SendMod::loop() {
}
