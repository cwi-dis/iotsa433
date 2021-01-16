#include "iotsa.h"
#include "iotsa433ReceiveForwarder.h"
#include <RCSwitch.h>

bool Iotsa433ReveiveForwarder::configLoad(IotsaConfigFileLoad& cf, String& f_name) {
  cf.get(f_name + ".url", url, "");
  if (url == "") return false;
  cf.get(f_name + ".telegram_tristate", telegram_tristate, "");
  cf.get(f_name + ".brand", brand, "");
  cf.get(f_name + ".group", group, "");
  cf.get(f_name + ".appliance", appliance, "");
  cf.get(f_name + ".state", state, "");
  cf.get(f_name + ".parameters", parameters, false);
  return true;
}

void Iotsa433ReveiveForwarder::configSave(IotsaConfigFileSave& cf, String& f_name) {
  cf.put(f_name + ".url", url);
  cf.put(f_name + ".telegram_tristate", telegram_tristate);
  cf.put(f_name + ".brand", brand);
  cf.put(f_name + ".group", group);
  cf.put(f_name + ".appliance", appliance);
  cf.put(f_name + ".state", state);
  cf.put(f_name + ".parameters", (int)parameters);
}

void Iotsa433ReveiveForwarder::formHandlerTH(String& message) {
 message += "<th>URL</th><th>telegram_tristate</th><th>brand</th><th>group</th><th>appliance</th><th>state</th><th>parameters?</th>";
}

void Iotsa433ReveiveForwarder::formHandler(String& message) {
  message += "URL: <input name='url'><br>";
  message += "Filter on telegram_tristate: <input name='telegram_tristate'><br>";
  message += "Filter on brand: <input name='brand'><br>";
  message += "Filter on group: <input name='group'><br>";
  message += "Filter on state: <input name='state'><br>";
  message += "Add parameters to URL on reception: <input type='checkbox' name='parameters'><br>";
}

void Iotsa433ReveiveForwarder::formHandler(String& message, String& text, String& f_name) {
  IotsaSerial.println("Iotsa433ReveiveForwarder::formHandler not implemented");
}

void Iotsa433ReveiveForwarder::formHandlerTD(String& message) {
    message += "<td>";
    message += url;
    message += "</td><td>";
    message += telegram_tristate;
    message += "</td><td>";
    message += brand;
    message += "</td><td>";
    message += group;
    message += "</td><td>";
    message += appliance;
    message += "</td><td>";
    message += state;
    message += "</td><td>";
    message += parameters;
    message += "</td>";
}

#ifdef IOTSA_WITH_WEB
bool Iotsa433ReveiveForwarder::formArgHandler(IotsaWebServer *server, String f_name) {
  // f_name unused for this object.
  url = server->arg("url"); 
  telegram_tristate = server->arg("telegram_tristate"); 
  brand = server->arg("brand"); 
  group = server->arg("group"); 
  appliance = server->arg("appliance"); 
  state = server->arg("state"); 
  String parameters = server->arg("parameters"); 
  parameters = parameters != "" && parameters != "0";
  return true;
}

#endif
#ifdef IOTSA_WITH_API
void Iotsa433ReveiveForwarder::getHandler(JsonObject& reply) {
  reply["url"] = url;
  reply["telegram_tristate"] = telegram_tristate;
  reply["brand"] = brand;
  reply["group"] = group;
  reply["appliance"] = appliance;
  reply["state"] = state;
  reply["parameters"] = parameters;
}

bool Iotsa433ReveiveForwarder::putHandler(const JsonVariant& request) {
  if (!request.is<JsonObject>()) return false;
  bool any = false;
  const JsonObject& reqObj = request.as<JsonObject>();
  if (reqObj.containsKey("url")) {
    any = true;
    url = reqObj["url"].as<String>();
  }
  if (reqObj.containsKey("telegram_tristate")) {
    any = true;
    telegram_tristate = reqObj["telegram_tristate"].as<String>();
  }
  if (reqObj.containsKey("brand")) {
    any = true;
    brand = reqObj["brand"].as<String>();
  }
  if (reqObj.containsKey("group")) {
    any = true;
    group = reqObj["group"].as<String>();
  }
  if (reqObj.containsKey("appliance")) {
    any = true;
    appliance = reqObj["appliance"].as<String>();
  }
  if (reqObj.containsKey("state")) {
    any = true;
    state = reqObj["state"].as<String>();
  }
  if (reqObj.containsKey("parameters")) {
    any = true;
    parameters = reqObj["parameters"].as<int>();
  }
  return any;
}

#endif
bool Iotsa433ReveiveForwarder::matches(String& _tristate, String& _brand, String& _group, String& _appliance, String& _state) {
  if (telegram_tristate != "" && _tristate != telegram_tristate) return false;
  if (brand != "" && _brand != brand) return false;
  if (group != "" && _group != group) return false;
  if (appliance != "" && _appliance != appliance) return false;
  if (state != "" && _state != state) return false;
  return true;
}

bool Iotsa433ReveiveForwarder::send(String& _tristate, String& _brand, String& _group, String& _appliance, String& _state) {
  // This forwarder applies to this appliance press.
  String _url = url;
  if (parameters) {
    url += "?telegram_tristate=" + _tristate + "&brand=" + _brand + "&group=" + _group + "&appliance=" + _appliance + "&state=" + _state;
  }
  IFDEBUG IotsaSerial.print("433recv: GET ");
  IFDEBUG IotsaSerial.println(url);
  return true;
}
