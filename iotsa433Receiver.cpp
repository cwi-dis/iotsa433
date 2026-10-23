#include "iotsa.h"
#include "iotsa433Receiver.h"
#include "iotsaConfigFile.h"

#undef _debug_print_raw_codes

Iotsa433ReceiverMod::Iotsa433ReceiverMod(IotsaApplication &_app, RCSwitch &_rcswitch, int _pin)
: IotsaModule(_app),
  rcswitch(_rcswitch),
  pin(_pin)
#ifdef IOTSA_WITH_WEB
  , webLog(*this),
  webForwarders(forwarders, IotsaWebCollection::Add | IotsaWebCollection::Remove, IotsaWebCollection::Table, IotsaWebContext::Saved,
      nullptr,
      // A forwarder needs somewhere to forward to.
      [](Iotsa433Forwarder& f, IotsaWebContext& ctx) {
        if (f.url == "") ctx.error("A forwarder needs a URL.");
        return f.url != "";
      })
#endif
{
}

void Iotsa433ReceiverMod::setStatusCallbacks(Iotsa433ReceiverCallback _forwarded, Iotsa433ReceiverCallback _failed) {
  forwardedCallback = _forwarded;
  failedCallback = _failed;
}

void Iotsa433ReceiverMod::setup() {
  configLoad();
  rcswitch.enableReceive(pin);
  rcswitch.setReceiveTolerance(30);
}

void Iotsa433ReceiverMod::lateSetup() {
  api.setup("433receiver", true, true, true);
  name = "433receiver";
}

void Iotsa433ReceiverMod::configLoad() {
  IotsaConfigFileLoad cf("/config/433receiver.cfg");
  forwarders.clear();
  for(int i=0; ; i++) {
    Iotsa433Forwarder newForwarder;
    if (!newForwarder.configLoad(cf, String(i))) break;
    forwarders.push_back(newForwarder);
  }
}

void Iotsa433ReceiverMod::configSave() {
  IotsaConfigFileSave cf("/config/433receiver.cfg");
  int i = 0;
  for(auto& f: forwarders) {
    f.configSave(cf, String(i++));
  }
}

void Iotsa433ReceiverMod::loop() {
  if (rcswitch.available()) {
    uint32_t telegram_binary = rcswitch.getReceivedValue();
    int telegram_bits = rcswitch.getReceivedBitlength();
    int telegram_protocol = rcswitch.getReceivedProtocol();
    int telegram_pulsewidth = rcswitch.getReceivedDelay();
    IFDEBUG IotsaSerial.printf("433recv: ts=%lu msg=0x%x\n", millis(), telegram_binary);
#ifdef _debug_print_raw_codes
    {
      unsigned int *raw = rcswitch.getReceivedRawdata();
      IotsaSerial.printf("433recv raw %d telegram_bits: ", telegram_bits);
      for(int i=0; i<2*telegram_bits; i++) {
        IotsaSerial.print(raw[i]);
        IotsaSerial.print(" ");
      }
      IotsaSerial.println();
    }
#endif
    _received(telegram_binary, telegram_protocol, telegram_bits, telegram_pulsewidth);
    rcswitch.resetAvailable();
  }
  if (logForward != logIn) _forwardOne();
}

void Iotsa433ReceiverMod::_received(uint32_t telegram_binary, int telegram_protocol, int telegram_bits, int telegram_pulsewidth) {
  Iotsa433Telegram& t = telegrams[logIn];
  t.telegram_binary = telegram_binary;
  t.telegram_protocol = telegram_protocol;
  t.telegram_bits = telegram_bits;
  t.telegram_pulsewidth = telegram_pulsewidth;
  t.receivedAt = millis();
  t.status = Iotsa433Telegram::queued;
  logIn = _next(logIn);
  if (logIn == logOut) {
    // Full: the oldest is dropped, also if it wasn't forwarded yet.
    if (logForward == logOut) logForward = _next(logForward);
    logOut = _next(logOut);
  }
}

// Forward the oldest telegram not yet forwarded, to every forwarder that
// matches it (cwi-dis/iotsa433#2).
void Iotsa433ReceiverMod::_forwardOne() {
  Iotsa433Telegram& t = telegrams[logForward];
  logForward = _next(logForward);
  String tristate, brand, group, appliance, state;
  t.status = Iotsa433Telegram::ignored;
  if (!t.decode(tristate, brand, group, appliance, state)) return;
  for (auto& f: forwarders) {
    if (!f.matches(tristate, brand, group, appliance, state)) continue;
    if (f.send(tristate, brand, group, appliance, state)) {
      if (t.status == Iotsa433Telegram::ignored) t.status = Iotsa433Telegram::sent;
      if (forwardedCallback) forwardedCallback();
    } else {
      t.status = Iotsa433Telegram::failed;
      IFDEBUG IotsaSerial.println("forward failed");
      if (failedCallback) failedCallback();
    }
  }
}

void Iotsa433ReceiverMod::getHandler(IotsaApiContext& ctx) {
  JsonArray forwardersList = ctx.reply()["forwarders"].to<JsonArray>();
  int i = 0;
  for (auto& f: forwarders) {
    IotsaApiContext::Scope s(ctx, String(i++), JsonObject(), forwardersList.add<JsonObject>());
    f.getHandler(ctx);
  }
  JsonArray receivedList = ctx.reply()["received"].to<JsonArray>();
  for (i = 0; i < _logSize(); i++) {
    IotsaApiContext::Scope s(ctx, String(i), JsonObject(), receivedList.add<JsonObject>());
    _logAt(i).getHandler(ctx);
  }
}

// Replace all forwarders, so a GET can be PUT back (backup and restore). The
// log ("received") can't be changed.
void Iotsa433ReceiverMod::putHandler(IotsaApiContext& ctx) {
  IotsaApiContext::Change change(ctx, "forwarders", IotsaApiContext::Saved);
  if (!change) {
    if (!ctx.failed()) ctx.error(400, "forwarders missing");
    return;
  }
  JsonVariant list = ctx.request()["forwarders"];
  if (!list.is<JsonArray>()) {
    ctx.error(400, "forwarders: expected an array");
    return;
  }
  std::vector<Iotsa433Forwarder> newForwarders;
  int i = 0;
  for (JsonVariant v : list.as<JsonArray>()) {
    if (!v.is<JsonObject>()) {
      ctx.error(400, "forwarders: expected an object for each forwarder");
      return;
    }
    Iotsa433Forwarder f;
    IotsaApiContext::Scope s(ctx, "forwarders." + String(i++), v.as<JsonObject>(), JsonObject());
    f.putHandler(ctx);
    if (ctx.failed()) return;
    newForwarders.push_back(f);
  }
  forwarders = newForwarders;
  change.done();
}

// Add a forwarder.
void Iotsa433ReceiverMod::postHandler(IotsaApiContext& ctx) {
  IotsaApiContext::Change change(ctx, "url", IotsaApiContext::Saved);
  if (!change) {
    if (!ctx.failed()) ctx.error(400, "url missing");
    return;
  }
  Iotsa433Forwarder f;
  f.putHandler(ctx);
  if (!change) return;
  forwarders.push_back(f);
  change.done();
}

#ifdef IOTSA_WITH_WEB
void Iotsa433ReceiverMod::formHandler_args(IotsaWebContext& ctx) {
  IotsaWebContext::Scope s(ctx, "forwarders");
  webForwarders.formHandler_args(ctx);
}

void Iotsa433ReceiverMod::formHandler_fields(IotsaWebContext& ctx) {
  if (ctx.includesEveryday()) {
    ctx << "<h3>Recently received telegrams</h3>\n";
    IotsaWebContext::Scope s(ctx, "received");
    webLog.formHandler_fields(ctx);
    ctx << "<p><a href=\"/433receiver\">Refresh</a></p>\n";
  }
  if (ctx.includesSettings()) {
    ctx << "<h3>Forwarders</h3>\n<p>Every forwarder that matches a received telegram sends its request, in this order.</p>\n";
    IotsaWebContext::Scope s(ctx, "forwarders");
    webForwarders.formHandler_fields(ctx);
  }
}
#endif // IOTSA_WITH_WEB

String Iotsa433ReceiverMod::info() {
  return "<p>433 MHz receiver: see <a href=\"/433receiver\">/433receiver</a> for the received telegrams and the forwarders.</p>";
}
