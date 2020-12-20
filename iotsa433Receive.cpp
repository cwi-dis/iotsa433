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

#ifdef IOTSA_WITH_WEB
void
Iotsa433ReceiveMod::handler() {
  bool anyChanged = false;
  if( server->hasArg("argument")) {
    if (needsAuthentication()) return;
    argument = server->arg("argument");
    anyChanged = true;
  }
  if (anyChanged) configSave();

  String message = "<html><head><title>433 MHz receiver module</title><style>table, th, td {border: 1px solid black;padding:5px;border-collapse: collapse;}</style></head><body><h1>433MHz receiver module</h1>";
  message += "<form method='get'>Argument: <input name='argument' value='";
  message += htmlEncode(argument);
  message += "'><br><input type='submit'></form>";

  message += "<h2>Received codes</h2>";
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
  reply["argument"] = argument;
  return true;
}

bool Iotsa433ReceiveMod::putHandler(const char *path, const JsonVariant& request, JsonObject& reply) {
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
  cf.get("argument", argument, "");
 
}

void Iotsa433ReceiveMod::configSave() {
  IotsaConfigFileSave cf("/config/433receive.cfg");
  cf.put("argument", argument);
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
}

void Iotsa433ReceiveMod::_received(uint32_t code) {
    received_buffer[received_in].code = code;
    received_buffer[received_in].millis = millis();
    received_in = RB_INC(received_in);
    if (received_in == received_out) received_out = RB_INC(received_out);
}