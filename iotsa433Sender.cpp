#include "iotsa.h"
#include "iotsa433Sender.h"
#include "decode433.h"

// The brands this build can send by name, for the page and GET /api/433sender.
static const char * const brands[] = {
#ifdef WITH_HEMA
  "HEMA",
#endif
#ifdef WITH_ELRO_FLAMINGO
  "ELRO",
#endif
  nullptr
};

// s is not empty and has only characters from chars.
static bool _onlyOf(const String& s, const char *chars) {
  if (s.length() == 0) return false;
  for (unsigned int i = 0; i < s.length(); i++) {
    if (strchr(chars, s[i]) == nullptr) return false;
  }
  return true;
}

void Iotsa433SenderMod::setup() {
  rcswitch.enableTransmit(pin);
}

void Iotsa433SenderMod::lateSetup() {
  api.setup("433sender", true, true, true);
  name = "433sender";
#ifdef IOTSA_WITH_WEB
  addTrigger("433send", [this](IotsaWebContext& ctx) { sendTrigger(ctx); });
#endif
}

void Iotsa433SenderMod::loop() {
}

// No configLoad()/configSave(): nothing is kept over reboots.

void Iotsa433SenderMod::_setProtocol(int protocol, int pulsewidth) {
  if (pulsewidth > 0) {
    rcswitch.setProtocol(protocol, pulsewidth);
  } else {
    rcswitch.setProtocol(protocol);
  }
}

bool Iotsa433SenderMod::_send(const Iotsa433Command& cmd, String& error) {
  bool isSwitch = cmd.brand != "" || cmd.group != "" || cmd.appliance != "" || cmd.state != "";
  int kinds = (cmd.tristate != "") + (cmd.binary != "") + isSwitch;
  if (kinds != 1) {
    error = kinds ? "one command at a time: telegram_tristate, telegram_binary, or a switch command"
                  : "nothing to send: need telegram_tristate, telegram_binary, or group, appliance and state";
    return false;
  }
  int protocol = cmd.protocol;
#ifdef WITH_ELRO_FLAMINGO
  if (protocol <= 0 && cmd.brand == "ELRO") protocol = 13;
#endif
  if (protocol <= 0) protocol = 1;
  if (cmd.tristate != "") {
    if (!_onlyOf(cmd.tristate, "01F")) {
      error = "telegram_tristate: only 0, 1 and F";
      return false;
    }
    IFDEBUG IotsaSerial.printf("433send: tristate=%s\n", cmd.tristate.c_str());
    _setProtocol(protocol, cmd.pulsewidth);
    rcswitch.sendTriState(cmd.tristate.c_str());
    return true;
  }
  if (cmd.binary != "") {
    if (!_onlyOf(cmd.binary, "01")) {
      error = "telegram_binary: only 0 and 1";
      return false;
    }
    IFDEBUG IotsaSerial.printf("433send: binary=%s\n", cmd.binary.c_str());
    _setProtocol(protocol, cmd.pulsewidth);
    rcswitch.send(cmd.binary.c_str());
    return true;
  }
  if (cmd.group == "" || cmd.appliance == "" || cmd.state == "") {
    error = "a switch command needs group, appliance and state";
    return false;
  }
  bool on;
  if (cmd.state == "on" || cmd.state == "1" || cmd.state == "true") {
    on = true;
  } else if (cmd.state == "off" || cmd.state == "0" || cmd.state == "false") {
    on = false;
  } else {
    error = "state: on or off";
    return false;
  }
  _setProtocol(protocol, cmd.pulsewidth);
  return _sendSwitch(cmd, on, error);
}

bool Iotsa433SenderMod::_sendSwitch(const Iotsa433Command& cmd, bool on, String& error) {
#ifdef WITH_ELRO_FLAMINGO
  if (cmd.brand == "ELRO") {
    String telegram = encode433_elro(cmd.group, cmd.appliance, (int)on);
    IFDEBUG IotsaSerial.printf("433send: brand=ELRO group=%s appliance=%s state=%d\n", cmd.group.c_str(), cmd.appliance.c_str(), on);
    rcswitch.send(telegram.c_str());
    return true;
  }
#endif
  if (cmd.brand != "" && cmd.brand != "HEMA") {
    error = "unknown brand " + cmd.brand;
    return false;
  }
  // HEMA, the default: appliance A-E or 0-5, otherwise a bit pattern.
  int applianceNum = -1;
  if (cmd.appliance.length() == 1) {
    char c = cmd.appliance[0];
    if (c >= 'A' && c <= 'E') applianceNum = c - 'A' + 1;
    if (c >= '0' && c <= '5') applianceNum = c - '0';
  }
  if (applianceNum >= 0) {
    IFDEBUG IotsaSerial.printf("433send: group=%s appliance=%d state=%d\n", cmd.group.c_str(), applianceNum, on);
    if (on) {
      rcswitch.switchOn(cmd.group.c_str(), applianceNum);
    } else {
      rcswitch.switchOff(cmd.group.c_str(), applianceNum);
    }
  } else {
    IFDEBUG IotsaSerial.printf("433send: group=%s appliance=%s state=%d\n", cmd.group.c_str(), cmd.appliance.c_str(), on);
    if (on) {
      rcswitch.switchOn(cmd.group.c_str(), cmd.appliance.c_str());
    } else {
      rcswitch.switchOff(cmd.group.c_str(), cmd.appliance.c_str());
    }
  }
  return true;
}

//
// Reading a command: the same fields on the page, the trigger and the API.
// Only state differs: the API also takes true/false and 1/0 as JSON values.
//
#ifdef IOTSA_WITH_WEB
static void _readState(IotsaWebContext& ctx, String& state) {
  ctx.arg("state", state, IotsaContext::Transient);
}
#endif

static void _readState(IotsaApiContext& ctx, String& state) {
  if (!ctx.argAction("state", IotsaContext::Transient)) return;
  JsonVariant v = ctx.request()["state"];
  if (v.is<const char *>()) {
    state = v.as<const char *>();
  } else if (v.is<bool>()) {
    state = v.as<bool>() ? "on" : "off";
  } else if (v.is<long>()) {
    state = v.as<long>() ? "on" : "off";
  } else {
    ctx.error(400, "state: on or off");
  }
}

template<class Ctx> static void _readCommand(Ctx& ctx, Iotsa433Command& cmd) {
  ctx.arg("telegram_tristate", cmd.tristate, IotsaContext::Transient);
  ctx.arg("telegram_binary", cmd.binary, IotsaContext::Transient);
  ctx.arg("brand", cmd.brand, IotsaContext::Transient);
  ctx.arg("group", cmd.group, IotsaContext::Transient);
  ctx.arg("appliance", cmd.appliance, IotsaContext::Transient);
  _readState(ctx, cmd.state);
  ctx.arg("telegram_protocol", cmd.protocol, IotsaContext::Transient);
  ctx.arg("telegram_pulsewidth", cmd.pulsewidth, IotsaContext::Transient);
}

// The brands this build knows, so a client can tell.
void Iotsa433SenderMod::getHandler(IotsaApiContext& ctx) {
  JsonArray list = ctx.reply()["brands"].to<JsonArray>();
  for (int i = 0; brands[i]; i++) list.add(brands[i]);
}

// Sending changes no state: PUT and POST are the same.
void Iotsa433SenderMod::putHandler(IotsaApiContext& ctx) {
  Iotsa433Command cmd;
  _readCommand(ctx, cmd);
  if (ctx.failed()) return;
  String error;
  if (!_send(cmd, error)) ctx.error(400, error);
}

void Iotsa433SenderMod::postHandler(IotsaApiContext& ctx) {
  putHandler(ctx);
}

#ifdef IOTSA_WITH_WEB
// The page has three forms, one per kind of command, each in its own Scope
// ("switch.group", ...) with its own Send button.
static const char * const forms[] = {"switch", "tristate", "binary"};

void Iotsa433SenderMod::formHandler_args(IotsaWebContext& ctx) {
  for (auto f : forms) {
    IotsaWebContext::Scope s(ctx, f);
    if (!ctx.argAction("send", IotsaWebContext::Transient)) continue;
    Iotsa433Command cmd;
    _readCommand(ctx, cmd);
    if (ctx.failed()) return;
    String error;
    if (_send(cmd, error)) {
      ctx.feedback("Sent.");
    } else {
      ctx.error(error);
    }
    return;
  }
}

void Iotsa433SenderMod::formHandler_fields(IotsaWebContext& ctx) {
  if (!ctx.includesEveryday()) return;
  String brandHint = "empty for the default";
  for (int i = 0; brands[i]; i++) brandHint += String(", ") + brands[i];
  {
    IotsaWebContext::Scope s(ctx, "switch");
    IotsaWebContext::Form form(ctx, "Send", "Switch command", "send");
    ctx.showField("Brand", "brand", "", IotsaWebContext::Transient, brandHint.c_str());
    ctx.showField("Group", "group", "", IotsaWebContext::Transient, "the DIP switches on the device, like 01011");
    ctx.showField("Appliance", "appliance", "", IotsaWebContext::Transient, "the letter on the remote control, like A, or a bit pattern like 01000");
    ctx.showField("State", "state", "on", IotsaWebContext::Transient, "on or off");
    ctx.showField("Protocol", "telegram_protocol", 0, IotsaWebContext::Transient, "0: the brand's default");
    ctx.showField("Pulse width", "telegram_pulsewidth", 0, IotsaWebContext::Transient, "microseconds, 0: the protocol's default");
  }
  {
    IotsaWebContext::Scope s(ctx, "tristate");
    IotsaWebContext::Form form(ctx, "Send", "Tristate telegram", "send");
    ctx.showField("Telegram", "telegram_tristate", "", IotsaWebContext::Transient, "like 00000FFF0FF0");
    ctx.showField("Protocol", "telegram_protocol", 0, IotsaWebContext::Transient, "0: protocol 1; the receiver's log shows what a remote control uses");
    ctx.showField("Pulse width", "telegram_pulsewidth", 0, IotsaWebContext::Transient, "microseconds, 0: the protocol's default");
  }
  {
    IotsaWebContext::Scope s(ctx, "binary");
    IotsaWebContext::Form form(ctx, "Send", "Binary telegram", "send");
    ctx.showField("Telegram", "telegram_binary", "", IotsaWebContext::Transient, "like 010100000000000000010100");
    ctx.showField("Protocol", "telegram_protocol", 0, IotsaWebContext::Transient, "0: protocol 1; the receiver's log shows what a remote control uses");
    ctx.showField("Pulse width", "telegram_pulsewidth", 0, IotsaWebContext::Transient, "microseconds, 0: the protocol's default");
  }
}

// The trigger, /433send: the same fields, without a Send button. Answers OK,
// or the error.
void Iotsa433SenderMod::sendTrigger(IotsaWebContext& ctx) {
  Iotsa433Command cmd;
  _readCommand(ctx, cmd);
  if (ctx.failed()) return;
  String error;
  if (!_send(cmd, error)) ctx.error(error);
}
#endif // IOTSA_WITH_WEB

String Iotsa433SenderMod::info() {
  return "<p>433 MHz sender: see <a href=\"/433sender\">/433sender</a>, or call /433send for a script or another device.</p>";
}
