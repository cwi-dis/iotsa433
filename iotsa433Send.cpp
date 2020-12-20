#include "iotsa.h"
#include "iotsa433Send.h"
#include "iotsaConfigFile.h"

#ifdef IOTSA_WITH_WEB
void
Iotsa433SendMod::handler() {
  bool anyChanged = false;
  if( server->hasArg("argument")) {
    if (needsAuthentication()) return;
    argument = server->arg("argument");
    anyChanged = true;
  }
  if (anyChanged) configSave();

  String message = "<html><head><title>433MHz sender module</title></head><body><h1>433 MHz sender module</h1>";
  message += "<form method='get'>Argument: <input name='argument' value='";
  message += htmlEncode(argument);
  message += "'><br><input type='submit'></form>";
  server->send(200, "text/html", message);
}

String Iotsa433SendMod::info() {
  String message = "<p>Built with 433 MHz sender module. See <a href=\"/433send\">/433send</a> to change the devices.</p>";
  return message;
}
#endif // IOTSA_WITH_WEB

void Iotsa433SendMod::setup() {
  configLoad();
}

#ifdef IOTSA_WITH_API
bool Iotsa433SendMod::getHandler(const char *path, JsonObject& reply) {
  reply["argument"] = argument;
  return true;
}

bool Iotsa433SendMod::putHandler(const char *path, const JsonVariant& request, JsonObject& reply) {
  bool anyChanged = false;
  JsonObject reqObj = request.as<JsonObject>();
  if (reqObj.containsKey("argument")) {
    argument = reqObj["argument"].as<String>();
    anyChanged = true;
  }
  if (anyChanged) configSave();
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
  cf.get("argument", argument, "");
 
}

void Iotsa433SendMod::configSave() {
  IotsaConfigFileSave cf("/config/433send.cfg");
  cf.put("argument", argument);
}

void Iotsa433SendMod::loop() {
}
