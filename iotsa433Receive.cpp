#include "iotsa.h"
#include "iotsa433Receive.h"
#include <RCSwitch.h>
#include "decode433.h"

#undef _debug_print_raw_codes

RCSwitch switch433; // Note: shared variable with Iotsa433Send
int switch433_pin_receive = 4;

struct received {
  uint32_t millis;
  uint32_t code;
  int protocol;
  int bits;
  int bitTime;
};

#define RB_SIZE 16
#define RB_INC(x) (((x)+1) % RB_SIZE)
static struct received received_buffer[RB_SIZE];
static int received_in = 0;
static int received_out = 0;
static int received_forward = 0;

static String _int2bin(int value, int bits) {
  String rv = String(value, BIN);
  while (rv.length() != (unsigned)bits) rv = "0" + rv;
  return rv;
}

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
  message += "<table><tr><th>seconds ago</th><th>decimal</th><th>binary</th><th>protocol</th><th>bits</th><th>bitTime</th><th>tristate</th><th>brand</th><th>dip switches</th><th>button</th><th>on/off</th></tr>";
  for(int i = received_out; i != received_in; i = RB_INC(i)) {
    message += "<tr><td>"  + String((millis()-received_buffer[i].millis)/1000.0) + "</td>";
    message += "<td>" + String(received_buffer[i].code) + "</td>";
    message += "<td>" + _int2bin(received_buffer[i].code, received_buffer[i].bits) + "</td>";
    message += "<td>" + String(received_buffer[i].protocol) + "</td>";
    message += "<td>" + String(received_buffer[i].bits) + "</td>";
    message += "<td>" + String(received_buffer[i].bitTime) + "</td>";
    message += "<td>";
    String tri_buf;
    if (decode433_tristate(received_buffer[i].code, received_buffer[i].bits, tri_buf)) {
      message += "<code>" + tri_buf + "</code>";
    } else {
      message += "not decodable";
    }
    message += "</td>";
    String dip_buf;
    String button_buf;
    String onoff_buf;
  #ifdef WITH_HEMA
    if (decode433_hema(received_buffer[i].code, received_buffer[i].bits, dip_buf, button_buf, onoff_buf)) {
      message += "<td>HEMA</td>";
      message += "</td><td>";
      message += dip_buf;
      message += "</td><td>";
      message += button_buf;
      message += "</td><td>";
      message += onoff_buf;
      message += "</td></tr>";
    } else 
#endif
#ifdef WITH_ELRO_FLAMINGO
    if (decode433_elro(received_buffer[i].code, received_buffer[i].bits, dip_buf, button_buf, onoff_buf)) {
      message += "<td>ELRO</td>";
      message += "</td><td>";
      message += dip_buf;
      message += "</td><td>";
      message += button_buf;
      message += "</td><td>";
      message += onoff_buf;
      message += "</td></tr>";
    } else 
#endif
    {
      message += "<td>unknown</td>";
      message += "</td><td>";
      message += "</td><td>";
      message += "</td><td>";
      message += "</td></tr>";
    }
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
  switch433.setReceiveTolerance(30);
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
    
    fRv["time"] = (millis() - received_buffer[i].millis)/1000.0;
    fRv["protocol"] = received_buffer[i].protocol;
    fRv["bits"] = received_buffer[i].bits;
    fRv["bitTime"] = received_buffer[i].bitTime;
    String tri_buf;
    if (decode433_tristate(received_buffer[i].code, received_buffer[i].bits, tri_buf)) fRv["tristate"] = tri_buf;
    fRv["binary"] = _int2bin(received_buffer[i].code, received_buffer[i].bits);
    String dip_buf;
    String button_buf;
    String onoff_buf;
#ifdef WITH_HEMA
    bool is_hema = decode433_hema(received_buffer[i].code, 24, dip_buf, button_buf, onoff_buf);
    if (is_hema) {
      fRv["brand"] = "HEMA";
      fRv["dipswitches"] = dip_buf;
      fRv["button"] = button_buf;
      fRv["onoff"] = onoff_buf;
    }
#endif
#ifdef WITH_ELRO_FLAMINGO
    bool is_elro = decode433_elro(received_buffer[i].code, 24, dip_buf, button_buf, onoff_buf);
    if (is_elro) {
      fRv["brand"] = "ELRO";
      fRv["dipswitches"] = dip_buf;
      fRv["button"] = button_buf;
      fRv["onoff"] = onoff_buf;
    }
#endif
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
#else
  IotsaSerial.println("PUT not yet imeplemented");
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
    int bits = switch433.getReceivedBitlength();
    int protocol = switch433.getReceivedProtocol();
    int bitTime = switch433.getReceivedDelay();
    IFDEBUG IotsaSerial.printf("433recv: ts=%lu msg=0x%x\n", millis(), code);
#ifdef _debug_print_raw_codes
    {
      unsigned int *raw = switch433.getReceivedRawdata();
      IotsaSerial.printf("433recv raw %d bits: ", bits);
      for(int i=0; i<2*bits; i++) {
        IotsaSerial.print(raw[i]);
        IotsaSerial.print(" ");
      }
      IotsaSerial.println();
    }
#endif
    _received(code, protocol, bits, bitTime);
    switch433.resetAvailable();
  }
  if (received_forward != received_in && !forwarders.empty()) {
    _forward_one();
  }
}

void Iotsa433ReceiveMod::_received(uint32_t code, int protocol, int bits, int bitTime) {
    received_buffer[received_in].code = code;
    received_buffer[received_in].protocol = protocol;
    received_buffer[received_in].bits = bits;
    received_buffer[received_in].bitTime = bitTime;
    received_buffer[received_in].millis = millis();
    received_in = RB_INC(received_in);
    if (received_in == received_out) received_out = RB_INC(received_out);
}

void Iotsa433ReceiveMod::_forward_one() {
  String tristate;
  if (!decode433_tristate(received_buffer[received_forward].code, received_buffer[received_forward].bits, tristate)) {
    IotsaSerial.println("433recv: ignore non-tristate code");
    received_forward = RB_INC(received_forward);
    return;
  }
  String brand = "unknown";
  String dipswitches;
  String button;
  String onoff;
#ifdef WITH_HEMA
  if( decode433_hema(received_buffer[received_forward].code, received_buffer[received_forward].bits, dipswitches, button, onoff)) {
    brand = "HEMA";
  }
#endif
#ifdef WITH_ELRO_FLAMINGO
  if( decode433_elro(received_buffer[received_forward].code, received_buffer[received_forward].bits, dipswitches, button, onoff)) {
    brand = "ELRO";
  }
#endif
  received_forward = RB_INC(received_forward);

  for (auto it: forwarders) {
    if (it.matches(tristate, brand, dipswitches, button, onoff)) {
      bool ok = it.send(tristate, brand, dipswitches, button, onoff);
      if (!ok) {
        IFDEBUG IotsaSerial.println("forward failed");
      }
      break;
    }
  }
}