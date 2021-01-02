#include "iotsa.h"
#include "iotsa433Receive.h"
#include <RCSwitch.h>
#include "decode433.h"

RCSwitch switch433; // Note: shared variable with Iotsa433Send
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

bool Iotsa433ReceiveMod::_addForwarder(Iotsa433ReveiveForwarder& newForwarder) {
  forwarders.push_back(newForwarder);
  return true;
}

bool Iotsa433ReceiveMod::_delForwarder(int index) {
  forwarders.erase(forwarders.begin()+index);
  return true;
}

bool Iotsa433ReceiveMod::_swapForwarder(int oldIndex, int newIndex) {
  Iotsa433ReveiveForwarder tmp = forwarders[oldIndex];
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
      Iotsa433ReveiveForwarder newfw;
      newfw.formArgHandler(server, "");
      anyChanged = _addForwarder(newfw);
    } else
    if (command == "Delete") {
      int index = server->arg("index").toInt();
      anyChanged = _delForwarder(index);
    } else
    if (command == "Up") {
      int index = server->arg("index").toInt();
      anyChanged = _swapForwarder(index, index-1);
    } else
    if (command == "Down") {
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
  message += "<table><tr><th>index</th>";
  Iotsa433ReveiveForwarder::formHandlerTH(message);
  message += "<th>OP</th></tr>";
  int i = 0;
  int last_i = forwarders.size()-1;
  for(auto it: forwarders) {
    // First field: index
    message += "<tr><td>" + String(i) + "</td>";
    // Subsequent fields: from forwarder433
    it.formHandlerTD(message);
    // Last field: operations
    message += "<td>";
    message += "<form><input type='hidden' name='index' value='" + String(i) + "'><input type='submit' name='command' value='Delete'>";
    if (i != 0) message += "<br><input type='submit' name='command' value='Up'>";
    if (i != last_i) message += "<br><input type='submit' name='command' value='Down'>";;
    message += "</form>";
    message += "</td></tr>";
    i++;
  }
  message += "</table>";
  message += "<h3>Add Forwarder</h3></table>";
  message += "<form>";
  Iotsa433ReveiveForwarder::formHandler(message);
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
  JsonArray rvConfig = reply.createNestedArray("forwarders");
  for (auto it: forwarders) {
    JsonObject fRv = rvConfig.createNestedObject();
    it.getHandler(fRv);
  }
  JsonArray rvReceived = reply.createNestedArray("received");
  for(int i = received_out; i != received_in; i = RB_INC(i)) {
    JsonObject fRv = rvReceived.createNestedObject();
    
    fRv["millis"] = millis() - received_buffer[i].millis;
    String tri_buf;
    if (decode433_tristate(received_buffer[i].code, 24, tri_buf)) fRv["tristate"] = tri_buf;
    String dip_buf;
    String button_buf;
    String onoff_buf;
    bool is_hema = decode433_hema(received_buffer[i].code, 24, dip_buf, button_buf, onoff_buf);
    if (is_hema) fRv["brand"] = "HEMA";
    fRv["dipswitches"] = dip_buf;
    fRv["button"] = button_buf;
    fRv["onoff"] = onoff_buf;
  }
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

  forwarders.clear();
  for(int idx=0; ; idx++) {
    String f_name = String(idx);
    Iotsa433ReveiveForwarder newForwarder;
    if (!newForwarder.configLoad(cf, f_name)) break;
    _addForwarder(newForwarder);
  }
}

void Iotsa433ReceiveMod::configSave() {
  IotsaConfigFileSave cf("/config/433receive.cfg");

  int idx = 0;
  for(auto it: forwarders) {
    String f_name = String(idx++);
    it.configSave(cf, f_name);
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
  if (received_forward != received_in && !forwarders.empty()) {
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

  for (auto it: forwarders) {
    if (it.matches(tristate, brand, dipswitches, button, onoff)) {
      bool ok = it.send(tristate, brand, dipswitches, button, onoff);
      break;
    }
  }
}