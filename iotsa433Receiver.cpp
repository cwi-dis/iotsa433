#include "iotsa.h"
#include "iotsa433Receiver.h"
#include "iotsaConfigFile.h"

Iotsa433ReceiverMod::Iotsa433ReceiverMod(IotsaApplication &_app, RCSwitch &_rcswitch, int _pin)
: IotsaModule(_app),
  rcswitch(_rcswitch),
  pin(_pin)
{
}

int Iotsa433TelegramLog::size() {
  return _mod._logSize();
}

IotsaApiModObject* Iotsa433TelegramLog::at(int i) {
  return &_mod._logAt(i);
}

// A forwarder needs somewhere to forward to.
bool Iotsa433ForwarderList::accept(Iotsa433Forwarder& forwarder, IotsaContext& ctx) {
  if (forwarder.url == "") {
    ctx.error(400, "A forwarder needs a URL.");
    return false;
  }
  return true;
}

// The raw timings of a telegram, for the debug log.
static String _rawCodes(RCSwitch& rcswitch, int telegram_bits) {
  unsigned int *raw = rcswitch.getReceivedRawdata();
  String rv;
  for (int i = 0; i < 2 * telegram_bits; i++) {
    if (i) rv += " ";
    rv += String(raw[i]);
  }
  return rv;
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
  // The forwarders decide what this device does to other devices: not for
  // everyone on the network. The telegram log stays readable.
  changesNeedRight("433receiver");
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
    IOTSA_LOG_DEBUG("433receiver", "received 0x%x, %d bits: %s", (unsigned)telegram_binary, telegram_bits, _rawCodes(rcswitch, telegram_bits).c_str());
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
      IOTSA_LOG("433receiver", "forward failed");
      if (failedCallback) failedCallback();
    }
  }
}

// Both collections (cwi-dis/iotsa#309). Only the forwarders change: a
// restore that sends "received" back gets it reported as ignored.
void Iotsa433ReceiverMod::getHandler(IotsaApiContext& ctx) {
  forwarderList.getHandler(ctx);
  telegramLog.getHandler(ctx);
}

void Iotsa433ReceiverMod::putHandler(IotsaApiContext& ctx) {
  forwarderList.putHandler(ctx);
}

void Iotsa433ReceiverMod::postHandler(IotsaApiContext& ctx) {
  forwarderList.postHandler(ctx);
}

#ifdef IOTSA_WITH_WEB
void Iotsa433ReceiverMod::formHandler_args(IotsaWebContext& ctx) {
  IotsaWebContext::Scope s(ctx, "forwarders");
  forwarderList.formHandler_args(ctx);
}

void Iotsa433ReceiverMod::formHandler_fields(IotsaWebContext& ctx) {
  if (ctx.includesEveryday()) {
    ctx << "<h3>Recently received telegrams</h3>\n";
    IotsaWebContext::Scope s(ctx, "received");
    telegramLog.formHandler_fields(ctx);
    ctx << "<p><a href=\"/433receiver\">Refresh</a></p>\n";
  }
  if (ctx.includesSettings()) {
    ctx << "<h3>Forwarders</h3>\n<p>Every forwarder that matches a received telegram sends its request, in this order.</p>\n";
    IotsaWebContext::Scope s(ctx, "forwarders");
    forwarderList.formHandler_fields(ctx);
  }
}
#endif // IOTSA_WITH_WEB

String Iotsa433ReceiverMod::info() {
  return "<p>433 MHz receiver: see <a href=\"/433receiver\">/433receiver</a> for the received telegrams and the forwarders.</p>";
}
