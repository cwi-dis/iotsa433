#include "iotsa.h"
#include "iotsa433Receive.h"
#include "iotsa433Telegram.h"
#include <RCSwitch.h>
#include "decode433.h"

#undef _debug_print_raw_codes

RCSwitch switch433; // Note: shared variable with Iotsa433Send
int switch433_pin_receive = 4;

//
// We maintain a static circular buffer of received telegrams
// (expected to be more efficient than C++ data structures)
//
#define RB_SIZE 16
#define RB_INC(x) (((x)+1) % RB_SIZE)
static Iotsa433Telegram received_buffer[RB_SIZE];
static int received_in = 0;
static int received_out = 0;
static int received_forward = 0;

void Iotsa433ReceiveMod::setStatusCallbacks(Iotsa433ReceiveCallback _ok, Iotsa433ReceiveCallback _notok) {
   statusOkCallback = _ok; 
   statusNotOkCallback = _notok; 
}

bool Iotsa433ReceiveMod::_addForwarder(Iotsa433ReceiveForwarder& newForwarder) {
  forwarders.push_back(newForwarder);
  return true;
}

bool Iotsa433ReceiveMod::_delForwarder(int index) {
  forwarders.erase(forwarders.begin()+index);
  return true;
}

bool Iotsa433ReceiveMod::_swapForwarder(int oldIndex, int newIndex) {
  Iotsa433ReceiveForwarder tmp = forwarders[oldIndex];
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
      Iotsa433ReceiveForwarder newfw;
      newfw.formHandler_args(server, "", true);
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
  message += "<h2>Configuration</h2><h3>Defined forwarders</h3>";
  message += "<table><tr><th>index</th>";
  Iotsa433ReceiveForwarder::formHandler_TH(message, true);
  message += "<th>OP</th></tr>";
  int i = 0;
  int last_i = forwarders.size()-1;
  for(auto it: forwarders) {
    // First field: index
    message += "<tr><td>" + String(i) + "</td>";
    // Subsequent fields: from forwarder433
    it.formHandler_TD(message, true);
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
  Iotsa433ReceiveForwarder::formHandler_emptyfields(message);
  message += "<br><input type='submit' name='command' value='Add'></form>";
  // Operation
  message += "<h2>Recently received codes</h2>";
  message += "<table><tr>";
  Iotsa433Telegram::formHandler_TH(message, false);
  for(int i = received_out; i != received_in; i = RB_INC(i)) {
    message += "<tr>";
    received_buffer[i].formHandler_TD(message, false);
    message += "</tr>";
  }
  message += "</table>";
  message += "<br><form method='get'><input type='submit' value='refresh'></form>";
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
  JsonArray rvConfig = reply["forwarders"].to<JsonArray>();
  for (auto it: forwarders) {
    JsonObject fRv = rvConfig.add<JsonObject>();
    it.getHandler(fRv);
  }
  JsonArray rvReceived = reply["received"].to<JsonArray>();
  for(int i = received_out; i != received_in; i = RB_INC(i)) {
    JsonObject fRv = rvReceived.add<JsonObject>();
    received_buffer[i].getHandler(fRv);

  }
  return true;
}

bool Iotsa433ReceiveMod::putHandler(const char *path, const JsonVariant& request, JsonObject& reply) {
  bool anyChanged = false;
#if 0
  JsonObject reqObj = request.as<JsonObject>();
  if (anyChanged) configSave();
#else
  IotsaSerial.println("PUT not yet implemented");
#endif
  return anyChanged;
}

bool Iotsa433ReceiveMod::postHandler(const char *path, const JsonVariant& request, JsonObject& reply) {
  bool anyChanged = false;
  JsonObject reqObj = request.as<JsonObject>();
  Iotsa433ReceiveForwarder newForwarder;
  if (newForwarder.putHandler(reqObj)) {
    if (_addForwarder(newForwarder)) {
      anyChanged = true;
      configSave();
    }
  }
  return anyChanged;
}
#endif // IOTSA_WITH_API

void Iotsa433ReceiveMod::serverSetup() {
#ifdef IOTSA_WITH_WEB
  server->on("/433receive", std::bind(&Iotsa433ReceiveMod::handler, this));
#endif
#ifdef IOTSA_WITH_API
  api.setup("/api/433receive", true, true, true);
  name = "433receive";
#endif
}

void Iotsa433ReceiveMod::configLoad() {
  IotsaConfigFileLoad cf("/config/433receive.cfg");

  forwarders.clear();
  for(int idx=0; ; idx++) {
    String f_name = String(idx);
    Iotsa433ReceiveForwarder newForwarder;
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
  IotsaSerial.printf("xxxjack configSave: %d items\n", idx);
}

void Iotsa433ReceiveMod::loop() {
  if (switch433.available()) {
    uint32_t telegram_binary = switch433.getReceivedValue();
    int telegram_bits = switch433.getReceivedBitlength();
    int telegram_protocol = switch433.getReceivedProtocol();
    int telegram_pulsewidth = switch433.getReceivedDelay();
    IFDEBUG IotsaSerial.printf("433recv: ts=%lu msg=0x%x\n", millis(), telegram_binary);
#ifdef _debug_print_raw_codes
    {
      unsigned int *raw = switch433.getReceivedRawdata();
      IotsaSerial.printf("433recv raw %d telegram_bits: ", telegram_bits);
      for(int i=0; i<2*telegram_bits; i++) {
        IotsaSerial.print(raw[i]);
        IotsaSerial.print(" ");
      }
      IotsaSerial.println();
    }
#endif
    _received(telegram_binary, telegram_protocol, telegram_bits, telegram_pulsewidth);
    switch433.resetAvailable();
  }
  if (received_forward != received_in && !forwarders.empty()) {
    _forward_one();
  }
}

void Iotsa433ReceiveMod::_received(uint32_t telegram_binary, int telegram_protocol, int telegram_bits, int telegram_pulsewidth) {
    received_buffer[received_in].telegram_binary = telegram_binary;
    received_buffer[received_in].telegram_protocol = telegram_protocol;
    received_buffer[received_in].telegram_bits = telegram_bits;
    received_buffer[received_in].telegram_pulsewidth = telegram_pulsewidth;
    received_buffer[received_in].millis = millis();
    received_in = RB_INC(received_in);
    if (received_in == received_out) received_out = RB_INC(received_out);
    // Presume we won't overrun received_forward...
}

void Iotsa433ReceiveMod::_forward_one() {
  String brand = "unknown";
  String group;
  String appliance;
  String state;
  String telegram_tristate;
  int cur_forward = received_forward;
  bool ok = received_buffer[received_forward]._parse(telegram_tristate, brand, group, appliance, state);
  received_forward = RB_INC(received_forward);
  received_buffer[received_forward].status = Iotsa433Telegram::TelegramStatus::queued;
  if (!ok) {
    received_buffer[cur_forward].status = Iotsa433Telegram::TelegramStatus::ignored;
    return;
  }

  for (auto it: forwarders) {
    if (it.matches(telegram_tristate, brand, group, appliance, state)) {
      received_buffer[cur_forward].status = Iotsa433Telegram::TelegramStatus::sending;
      ok = it.send(telegram_tristate, brand, group, appliance, state);
      if (ok) {
        received_buffer[cur_forward].status = Iotsa433Telegram::TelegramStatus::sent_ok;
        if (statusOkCallback != nullptr) statusOkCallback();
      } else {
        received_buffer[cur_forward].status = Iotsa433Telegram::TelegramStatus::sent_fail;
        IFDEBUG IotsaSerial.println("forward failed");
        if (statusNotOkCallback != nullptr) statusNotOkCallback();
      }
      return;
    }
  }
  received_buffer[cur_forward].status = Iotsa433Telegram::TelegramStatus::ignored;
}