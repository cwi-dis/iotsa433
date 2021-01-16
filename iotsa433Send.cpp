#include "iotsa.h"
#include "iotsa433Send.h"
#include "iotsaConfigFile.h"
#include <RCSwitch.h>
#include "decode433.h"

extern RCSwitch switch433; // Note: shared variable with Iotsa433Receive
int switch433_pin_send = 5;

bool Iotsa433SendMod::_send_binary(int telegram_protocol, int telegram_pulsewidth, String telegram_binary) {
  if (telegram_protocol < 0) telegram_protocol = 1; // Default telegram_protocol
  if (telegram_pulsewidth > 0) {
    switch433.setProtocol(telegram_protocol, telegram_pulsewidth); // Set telegram_protocol, override telegram_pulsewidth
  } else {
    switch433.setProtocol(telegram_protocol); // Set telegram_protocol, use default telegram_pulsewidth
  }
  switch433.send(telegram_binary.c_str());
  return true;
}

bool Iotsa433SendMod::_send_tristate(int telegram_protocol, int telegram_pulsewidth, String telegram_tristate) {
  if (telegram_protocol < 0) telegram_protocol = 1; // Default telegram_protocol
  if (telegram_pulsewidth > 0) {
    switch433.setProtocol(telegram_protocol, telegram_pulsewidth); // Set telegram_protocol, override telegram_pulsewidth
  } else {
    switch433.setProtocol(telegram_protocol); // Set telegram_protocol, use default telegram_pulsewidth
  }
  switch433.sendTriState(telegram_tristate.c_str());
  return true;
}

bool Iotsa433SendMod::_send_brand(int telegram_protocol, int telegram_pulsewidth, String brand, String group, String appliance, bool state) {
  if (telegram_protocol <= 0) {
    telegram_protocol = 1; // Default telegram_protocol
#ifdef WITH_ELRO_FLAMINGO
    if (brand == "ELRO") telegram_protocol = 13;
#endif
  }
  if (telegram_pulsewidth > 0) {
    switch433.setProtocol(telegram_protocol, telegram_pulsewidth); // Set telegram_protocol, override telegram_pulsewidth
  } else {
    switch433.setProtocol(telegram_protocol); // Set telegram_protocol, use default telegram_pulsewidth
  }
#ifdef WITH_HEMA
  if (brand == "HEMA") brand = ""; // default behaviour
#endif
#ifdef WITH_ELRO_FLAMINGO
  if (brand == "ELRO") {
    String telegram_binary = encode433_elro(group, appliance, (int)state);
    switch433.send(telegram_binary.c_str());
    return true;
  }
#endif
  if (brand == "") {
    // Default treatment. We support ABCDE or 12345, otherwise appliance is a bitstring.
    int applianceNum = -1;
    if (appliance == "A" || appliance == "B" || appliance == "C" || appliance == "D" || appliance == "E") {
      // Hema-style device
      applianceNum = appliance[0] - 'A';
    }
    if (appliance == "0" || appliance == "1" || appliance == "2" || appliance == "3" || appliance == "4" || appliance == "5") {
      // Numbered appliances
      applianceNum = appliance[0] - '0';
    }
    if (applianceNum >= 0) {
      if (state) {
        switch433.switchOn(group.c_str(), applianceNum);
      } else {
        switch433.switchOff(group.c_str(), applianceNum);
      }
    } else {
      // Presume appliance is a mask-style string
      if (state) {
        switch433.switchOn(group.c_str(), appliance.c_str());
      } else {
        switch433.switchOff(group.c_str(), appliance.c_str());
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
  int telegram_protocol = -1;
  int telegram_pulsewidth = -1;
  if (server->hasArg("telegram_protocol")) {
    telegram_protocol = server->arg("telegram_protocol").toInt();
  }
  if (server->hasArg("telegram_pulsewidth")) {
    telegram_pulsewidth = server->arg("telegram_pulsewidth").toInt();
  }

  if (server->hasArg("telegram_tristate")) {
    // Send telegram_tristate command
    String telegram_tristate = server->arg("telegram_tristate");
    if (_send_tristate(telegram_protocol, telegram_pulsewidth, telegram_tristate)) {
      server->send(200, "text/plain", "OK");
    } else {
      server->send(400, "text/plain", "Bad telegram_tristate value");
    }
    return;
  }

  if (server->hasArg("telegram_binary")) {
    // Send binary command
    String telegram_binary = server->arg("telegram_binary");
    if (_send_binary(telegram_protocol, telegram_pulsewidth, telegram_binary)) {
      server->send(200, "text/plain", "OK");
    } else {
      server->send(400, "text/plain", "Bad binary value");
    }
    return;
  }

  String brand;
  if (server->hasArg("brand")) brand = server->arg("brand");
  int nArgs = 0;

  if (server->hasArg("state")) nArgs++;
  if (server->hasArg("group")) nArgs++;
  if (server->hasArg("appliance")) nArgs++;
  if (nArgs == 3) {
    // We have all arguments. Send command.
    String group = server->arg("group");
    String appliance = server->arg("appliance");
    String state = server->arg("state");
    bool on = (state == "on" || state == "1");
    if (!on && ! (state == "off" || state == "0" || state == "")) {
      server->send(400, "text/plain", "Bad state value");
      return;
    }

    if (_send_brand(telegram_protocol, telegram_pulsewidth, brand, group, appliance, on))  {
      server->send(200, "text/plain", "OK");
    } else {
      server->send(400, "text/plain", "Bad command");
    }
    return;
  } else
  if (nArgs != 0) {
    // Some but not all arguments. Error.
    server->send(400, "text/plain", "Must have all of state, group, appliance");
    return;
  }
  //
  // No command found. Present form for user input.
  //
  String message = "<html><head><title>433MHz sender module</title></head><body><h1>433 MHz sender module</h1>";

  message += "<h2>Send Command</h2><form method='get'><table>";
  message += "<tr><td>Brand:</td><td><input name='brand'></td>";
  message += "<td><i>(example: HEMA, ELRO, empty)</i></td></tr>";
  message += "<tr><td>group:</td><td><input name='group'></td>";
  message += "<td><i>(example: 01011, how DIP switches on device are set)</i></td></tr>";
  message += "<tr><td>appliance:</td><td><input name='appliance'></td>";
  message += "<td><i>(example: A, label on remote control appliance, or 01000, binary bitpattern)</i></td></tr>";
  message += "<tr><td>On/Off:</td><td><input name='state'></td>";
  message += "<td><i>(example: on or off)</i><br></td></tr>";
  message += "<tr><td>telegram_protocol:</td><td><input name='telegram_protocol' value='1'></td>";
  message += "<td><i>(433 bit telegram_protocol, usually 1, see receiver dump)</i><br></td></tr>";
  message += "<tr><td>telegram_pulsewidth:</td><td><input name='telegram_pulsewidth' value='300'></td>";
  message += "<td><i>(pulse duration in microseconds, see receiver dump)</i><br></td></tr>";
  message += "<tr><td></td><td><input type='submit' value='Send Switch Command'></td></tr></table></form>";

  message += "<h2>Send low-level tristate telegram</h2><form method='get'>";
  message += "Tristate command: <input name='telegram_tristate'>";
  message += "<i>(example: 00000FFF0FF0)</i><br>";
  message += "telegram_protocol: <input name='telegram_protocol' value='1'><br>";
  message += "telegram_pulsewidth: <input name='telegram_pulsewidth' value='300'><br>";
  message += "<input type='submit' value='Send telegram_tristate'></form>";

  message += "<h2>Send low-level binary telegram</h2><form method='get'>";
  message += "Binary command: <input name='telegram_binary'>";
  message += "<i>(example: 010100000000000000010100)</i><br>";
  message += "telegram_protocol: <input name='telegram_protocol' value='1'><br>";
  message += "telegram_pulsewidth: <input name='telegram_pulsewidth' value='300'><br>";
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
