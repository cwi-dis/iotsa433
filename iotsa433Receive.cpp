#include "iotsa.h"
#include "iotsa433Receive.h"
#include "iotsaConfigFile.h"
#include <RCSwitch.h>
#include "decode433.h"

RCSwitch switch433;
int switch433_pin_receive = 4;

struct received {
  uint32_t millis;
  uint32_t code;
};

#define RB_SIZE 8
#define RB_INC(x) (((x)+1) % RB_SIZE)
static struct received received_buffer[RB_SIZE];
static int received_in = 0;
static int received_out = 0;
static int received_forward = 0;

bool Iotsa433ReceiveMod::_addForwarder(struct forwarder& newForwarder) {
  forwarders = (struct forwarder *)realloc(forwarders, (nForwarders+1)*sizeof(struct forwarder));
  if (forwarders == NULL) {
    IotsaSerial.println("iotsa433: _addForwarder: out of memory");
    return false;
  }
  for (int i=nForwarders; i > 0; i--) {
    forwarders[i] = forwarders[i-1];
  }
  memset(forwarders + 0, 0, sizeof(struct forwarder));
  forwarders[0] = newForwarder;
  nForwarders++;
  return true;
}

bool Iotsa433ReceiveMod::_delForwarder(int index) {
  for(int i = index+1; i < nForwarders; i++) {
    forwarders[i-1] = forwarders[i];
  }
  nForwarders--;
  if (nForwarders == 0) {
    free(forwarders);
    forwarders = NULL;
    return true;
  }
  forwarders = (struct forwarder *)realloc(forwarders, nForwarders*sizeof(struct forwarder));
  if (forwarders == NULL) {
    IotsaSerial.println("iotsa433: _delForwarder: out of memory");
    return false;
  }
  return true;
}

bool Iotsa433ReceiveMod::_swapForwarder(int oldIndex, int newIndex) {
  struct forwarder tmp = forwarders[oldIndex];
  forwarders[oldIndex] = forwarders[newIndex];
  forwarders[newIndex] = tmp;
  return true;
}

#ifdef IOTSA_WITH_WEB
void
Iotsa433ReceiveMod::handler() {
  bool anyChanged = false;
  if( server->hasArg("command")) {
    if (needsAuthentication()) return;
    String command = server->arg("command");
    if (command == "Add") {
      IFDEBUG IotsaSerial.println("xxxjack add");
      struct forwarder newfw;
      newfw.url = server->arg("url"); 
      newfw.tristate = server->arg("tristate"); 
      newfw.brand = server->arg("brand"); 
      newfw.dipswitches = server->arg("dipswitches"); 
      newfw.button = server->arg("button"); 
      newfw.onoff = server->arg("onoff"); 
      String parameters = server->arg("parameters"); 
      newfw.parameters = parameters != "" && parameters != "0";
      anyChanged = _addForwarder(newfw);
    } else
    if (command == "Delete") {
      IFDEBUG IotsaSerial.println("xxxjack delete");
      int index = server->arg("index").toInt();
      anyChanged = _delForwarder(index);
    } else
    if (command == "Up") {
      IFDEBUG IotsaSerial.println("xxxjack up");
      int index = server->arg("index").toInt();
      anyChanged = _swapForwarder(index, index-1);
    } else
    if (command == "Down") {
      IFDEBUG IotsaSerial.println("xxxjack down");
      int index = server->arg("index").toInt();
      anyChanged = _swapForwarder(index, index+1);
    }
  }
  if (anyChanged) configSave();

  String message = "<html><head><title>433 MHz receiver module</title><style>table, th, td {border: 1px solid black;padding:5px;border-collapse: collapse;}</style></head><body><h1>433MHz receiver module</h1>";
  // Configuration
#if 0
    String url; // URL to send a request to
    String tristate; // if non-empty, only apply to this tri-state code
    String brand; // if non-empty, only apply to switches of this brand
    String dipswitches; // if non-empty only apply to switch with this dip-switch selection
    String button;  // if non-empty only apply to this button name
    String onoff; // if non-empty only apply to this on/off setting
    bool parameters;  // if True, add URL parameters for each parameter/value
#endif
  message += "<h2>Configuration</h2><h3>Defined forwarders</h3>";
  message += "<table><tr><th>index</th><th>URL</th><th>tristate</th><th>brand</th><th>dipswitches</th><th>button</th><th>onoff</th><th>parameters?</th><th>OP</th></tr>";
  for (int i=0; i< nForwarders; i++) {
    struct forwarder *f = forwarders+i;
    message += "<tr><td>" + String(i) + "</td><td>";
    message += f->url;
    message += "</td><td>";
    message += f->tristate;
    message += "</td><td>";
    message += f->brand;
    message += "</td><td>";
    message += f->dipswitches;
    message += "</td><td>";
    message += f->button;
    message += "</td><td>";
    message += f->onoff;
    message += "</td><td>";
    message += f->parameters;
    message += "</td><td>";
    message += "<form><input type='hidden' name='index' value='" + String(i) + "'><input type='submit' name='command' value='Delete'>";
    if (i != 0) message += "<br><input type='submit' name='command' value='Up'>";
    if (i != nForwarders-1) message += "<br><input type='submit' name='command' value='Down'>";;
    message += "</form>";
    message += "</td></tr>";
  }
  message += "</table>";
  message += "<h3>Add Forwarder</h3></table>";
  message += "<form>";
  message += "URL: <input name='url'><br>";
  message += "Filter on tristate: <input name='tristate'><br>";
  message += "Filter on brand: <input name='brand'><br>";
  message += "Filter on dipswitches: <input name='dipswitches'><br>";
  message += "Filter on onoff: <input name='onoff'><br>";
  message += "Add parameters to URL on reception: <input type='checkbox' name='parameters'><br>";
  message += "<br><input type='submit' name='command' value='Add'></form>";
  // Operation
  message += "<h2>Recently received codes</h2>";
  message += "<table><tr><th>ms ago</th><th>decimal</th><th>tristate</th><th>brand</th><th>dip switches</th><th>button</th><th>on/off</th></tr>";
  for(int i = received_out; i != received_in; i = RB_INC(i)) {
    message += "<tr><td>"  + String(millis()-received_buffer[i].millis) + "</td><td>" + String(received_buffer[i].code) + "</td><td>";
    String tri_buf;
    if (decode433_tristate(received_buffer[i].code, 24, tri_buf)) {
      message += tri_buf;
    } else {
      message += "not decodable";
    }
    message += "</td>";
    String dip_buf;
    String button_buf;
    String onoff_buf;
    bool is_hema = decode433_hema(received_buffer[i].code, 24, dip_buf, button_buf, onoff_buf);
    if (is_hema) {
      message += "<td>HEMA</td>";
    } else {
      message += "<td>unknown</td>";
    }
    message += "</td><td>";
    message += dip_buf;
    message += "</td><td>";
    message += button_buf;
    message += "</td><td>";
    message += onoff_buf;
    message += "</td></tr>";
  }
  message += "</table>";
  server->send(200, "text/html", message);
}

String Iotsa433ReceiveMod::info() {
  String message = "<p>Built with 433MHz receiver module. See <a href=\"/433receive\">/433receive</a> to change the remote controls.</p>";
  return message;
}
#endif // IOTSA_WITH_WEB

void Iotsa433ReceiveMod::setup() {
  configLoad();
  switch433.enableReceive(switch433_pin_receive);
}

#ifdef IOTSA_WITH_API
bool Iotsa433ReceiveMod::getHandler(const char *path, JsonObject& reply) {
  return true;
}

bool Iotsa433ReceiveMod::putHandler(const char *path, const JsonVariant& request, JsonObject& reply) {
  bool anyChanged = false;
  JsonObject reqObj = request.as<JsonObject>();
#if 0
  if (reqObj.containsKey("argument")) {
    argument = reqObj["argument"].as<String>();
    anyChanged = true;
  }
#endif
  if (anyChanged) configSave();
  return anyChanged;
}
#endif // IOTSA_WITH_API

void Iotsa433ReceiveMod::serverSetup() {
#ifdef IOTSA_WITH_WEB
  server->on("/433receive", std::bind(&Iotsa433ReceiveMod::handler, this));
#endif
#ifdef IOTSA_WITH_API
  api.setup("/api/433receive", true, true);
  name = "433receive";
#endif
}

void Iotsa433ReceiveMod::configLoad() {
  IotsaConfigFileLoad cf("/config/433receive.cfg");
#if 0
    String tristate; // if non-empty, only apply to this tri-state code
    String brand; // if non-empty, only apply to switches of this brand
    String dipswitches; // if non-empty only apply to switch with this dip-switch selection
    String button;  // if non-empty only apply to this button name
    String onoff; // if non-empty only apply to this on/off setting
    String url; // URL to send a request to
    bool parameters;  // if True, add URL parameters for each parameter/value
#endif
  if (forwarders) free(forwarders);
  forwarders = 0;
  nForwarders = 0;
  cf.get("nForwarders", nForwarders, 0);
  if (nForwarders == 0) return;
  forwarders = (struct forwarder *)calloc(nForwarders, sizeof(struct forwarder));
  if (forwarders == NULL) {
    nForwarders = 0;
    IotsaSerial.println("iotsa433: configLoad: out of memory");
    return;
  }
  for(int i=0; i<nForwarders; i++) {
    String f_name = String(i);
    cf.get(f_name + ".url", forwarders[i].url, "");
    cf.get(f_name + ".tristate", forwarders[i].tristate, "");
    cf.get(f_name + ".brand", forwarders[i].brand, "");
    cf.get(f_name + ".dipswitches", forwarders[i].dipswitches, "");
    cf.get(f_name + ".button", forwarders[i].button, "");
    cf.get(f_name + ".onoff", forwarders[i].onoff, "");
    int intvalue;
    cf.get(f_name + ".parameters", intvalue, 0);
    forwarders[i].parameters = intvalue;
  }
}

void Iotsa433ReceiveMod::configSave() {
  IotsaConfigFileSave cf("/config/433receive.cfg");

  cf.put("nForwarders", nForwarders);
  for(int i=0; i<nForwarders; i++) {
    String f_name = String(i);
    cf.put(f_name + ".url", forwarders[i].url);
    cf.put(f_name + ".tristate", forwarders[i].tristate);
    cf.put(f_name + ".brand", forwarders[i].brand);
    cf.put(f_name + ".dipswitches", forwarders[i].dipswitches);
    cf.put(f_name + ".button", forwarders[i].button);
    cf.put(f_name + ".onoff", forwarders[i].onoff);
    cf.put(f_name + ".parameters", (int)forwarders[i].parameters);
  }
}

void Iotsa433ReceiveMod::loop() {
  if (switch433.available()) {
    uint32_t code = switch433.getReceivedValue();
    IFDEBUG IotsaSerial.printf("433recv: ts=%lu msg=0x%x\n", millis(), code);
    if (switch433.getReceivedBitlength() != 24) {
      IotsaSerial.println("433recv: ignore non-24-bit code");
    } else {
      _received(code);
    }
    switch433.resetAvailable();
  }
  if (received_forward != received_in && nForwarders > 0) {
    _forward_one();
  }
}

void Iotsa433ReceiveMod::_received(uint32_t code) {
    received_buffer[received_in].code = code;
    received_buffer[received_in].millis = millis();
    received_in = RB_INC(received_in);
    if (received_in == received_out) received_out = RB_INC(received_out);
}

void Iotsa433ReceiveMod::_forward_one() {
  String tristate;
  if (!decode433_tristate(received_buffer[received_forward].code, 24, tristate)) {
    IotsaSerial.println("433recv: ignore non-tristate code");
    received_forward = RB_INC(received_forward);
    return;
  }
  String brand = "unknown";
  String dipswitches;
  String button;
  String onoff;
  if( decode433_hema(received_buffer[received_forward].code, 24, dipswitches, button, onoff)) {
    brand = "HEMA";
  }
  received_forward = RB_INC(received_forward);

  for (int i=0; i< nForwarders; i++) {
    if (forwarders[i].tristate != "" && forwarders[i].tristate != tristate) continue;
    if (forwarders[i].brand != "" && forwarders[i].brand != brand) continue;
    if (forwarders[i].dipswitches != "" && forwarders[i].dipswitches != dipswitches) continue;
    if (forwarders[i].button != "" && forwarders[i].button != button) continue;
    if (forwarders[i].onoff != "" && forwarders[i].onoff != onoff) continue;
    // This forwarder applies to this button press.
    String url = forwarders[i].url;
    if (forwarders[i].parameters) {
      url += "?tristate=" + tristate + "&brand=" + brand + "&dipswitches=" + dipswitches + "&button=" + button + "&onoff=" + onoff;
    }
    IFDEBUG IotsaSerial.print("433recv: GET ");
    IFDEBUG IotsaSerial.println(url);
    break;
  }
}