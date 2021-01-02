#include "iotsa.h"
#include "iotsa433ReceiveForwarder.h"
#include <RCSwitch.h>

bool Iotsa433ReveiveForwarder::configLoad(IotsaConfigFileLoad& cf, String& f_name) {
  cf.get(f_name + ".url", url, "");
  if (url == "") return false;
  cf.get(f_name + ".tristate", tristate, "");
  cf.get(f_name + ".brand", brand, "");
  cf.get(f_name + ".dipswitches", dipswitches, "");
  cf.get(f_name + ".button", button, "");
  cf.get(f_name + ".onoff", onoff, "");
  cf.get(f_name + ".parameters", parameters, false);
  return true;
}

void Iotsa433ReveiveForwarder::configSave(IotsaConfigFileSave& cf, String& f_name) {
  cf.put(f_name + ".url", url);
  cf.put(f_name + ".tristate", tristate);
  cf.put(f_name + ".brand", brand);
  cf.put(f_name + ".dipswitches", dipswitches);
  cf.put(f_name + ".button", button);
  cf.put(f_name + ".onoff", onoff);
  cf.put(f_name + ".parameters", (int)parameters);
}

void Iotsa433ReveiveForwarder::formHandlerTH(String& message) {
 message += "<th>URL</th><th>tristate</th><th>brand</th><th>dipswitches</th><th>button</th><th>onoff</th><th>parameters?</th>";
}

void Iotsa433ReveiveForwarder::formHandler(String& message) {
  message += "URL: <input name='url'><br>";
  message += "Filter on tristate: <input name='tristate'><br>";
  message += "Filter on brand: <input name='brand'><br>";
  message += "Filter on dipswitches: <input name='dipswitches'><br>";
  message += "Filter on onoff: <input name='onoff'><br>";
  message += "Add parameters to URL on reception: <input type='checkbox' name='parameters'><br>";
}

void Iotsa433ReveiveForwarder::formHandler(String& message, String& text, String& f_name) {
  IotsaSerial.println("Iotsa433ReveiveForwarder::formHandler not implemented");
}

void Iotsa433ReveiveForwarder::formHandlerTD(String& message) {
    message += "<td>";
    message += url;
    message += "</td><td>";
    message += tristate;
    message += "</td><td>";
    message += brand;
    message += "</td><td>";
    message += dipswitches;
    message += "</td><td>";
    message += button;
    message += "</td><td>";
    message += onoff;
    message += "</td><td>";
    message += parameters;
    message += "</td>";
}

#ifdef IOTSA_WITH_WEB
bool Iotsa433ReveiveForwarder::formArgHandler(IotsaWebServer *server, String f_name) {
  // f_name unused for this object.
  url = server->arg("url"); 
  tristate = server->arg("tristate"); 
  brand = server->arg("brand"); 
  dipswitches = server->arg("dipswitches"); 
  button = server->arg("button"); 
  onoff = server->arg("onoff"); 
  String parameters = server->arg("parameters"); 
  parameters = parameters != "" && parameters != "0";
  return true;
}

#endif
#ifdef IOTSA_WITH_API
void Iotsa433ReveiveForwarder::getHandler(JsonObject& reply) {
  reply["url"] = url;
  reply["tristate"] = tristate;
  reply["brand"] = brand;
  reply["dipswitches"] = dipswitches;
  reply["button"] = button;
  reply["onoff"] = onoff;
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
  if (reqObj.containsKey("tristate")) {
    any = true;
    tristate = reqObj["tristate"].as<String>();
  }
  if (reqObj.containsKey("brand")) {
    any = true;
    brand = reqObj["brand"].as<String>();
  }
  if (reqObj.containsKey("dipswitches")) {
    any = true;
    dipswitches = reqObj["dipswitches"].as<String>();
  }
  if (reqObj.containsKey("button")) {
    any = true;
    button = reqObj["button"].as<String>();
  }
  if (reqObj.containsKey("onoff")) {
    any = true;
    onoff = reqObj["onoff"].as<String>();
  }
  if (reqObj.containsKey("parameters")) {
    any = true;
    parameters = reqObj["parameters"].as<int>();
  }
  return any;
}

#endif
bool Iotsa433ReveiveForwarder::matches(String& _tristate, String& _brand, String& _dipswitches, String& _button, String& _onoff) {
  if (tristate != "" && _tristate != tristate) return false;
  if (brand != "" && _brand != brand) return false;
  if (dipswitches != "" && _dipswitches != dipswitches) return false;
  if (button != "" && _button != button) return false;
  if (onoff != "" && _onoff != onoff) return false;
  return true;
}

bool Iotsa433ReveiveForwarder::send(String& _tristate, String& _brand, String& _dipswitches, String& _button, String& _onoff) {
  // This forwarder applies to this button press.
  String _url = url;
  if (parameters) {
    url += "?tristate=" + _tristate + "&brand=" + _brand + "&dipswitches=" + _dipswitches + "&button=" + _button + "&onoff=" + _onoff;
  }
  IFDEBUG IotsaSerial.print("433recv: GET ");
  IFDEBUG IotsaSerial.println(url);
  return true;
}
