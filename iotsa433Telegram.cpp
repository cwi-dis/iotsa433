#include "iotsa433Telegram.h"
#include "decode433.h"

static String _int2bin(uint32_t value, int telegram_bits) {
  String rv = String(value, BIN);
  while (rv.length() < (unsigned)telegram_bits) rv = "0" + rv;
  return rv;
}

const char *Iotsa433Telegram::statusName(Status s) {
  switch (s) {
    case queued: return "queued";
    case ignored: return "ignored";
    case sent: return "sent";
    case failed: return "failed";
  }
  return "?";
}

bool Iotsa433Telegram::decode(String& tristate, String& brand, String& group, String& appliance, String& state) {
  bool ok = decode433_tristate(telegram_binary, telegram_bits, tristate);
  brand = "unknown";
#ifdef WITH_HEMA
  if (decode433_hema(telegram_binary, telegram_bits, group, appliance, state)) {
    brand = "HEMA";
    return true;
  }
#endif
#ifdef WITH_ELRO_FLAMINGO
  if (decode433_elro(telegram_binary, telegram_bits, group, appliance, state)) {
    brand = "ELRO";
    return true;
  }
#endif
  return ok;
}

void Iotsa433Telegram::getHandler(IotsaApiContext& ctx) {
  String tristate, brand, group, appliance, state;
  decode(tristate, brand, group, appliance, state);
  ctx.show("time", (millis() - receivedAt)/1000.0);
  ctx.show("status", statusName(status));
  if (brand != "unknown") {
    ctx.show("brand", brand);
    ctx.show("group", group);
    ctx.show("appliance", appliance);
    ctx.show("state", state);
  }
  ctx.show("telegram_protocol", telegram_protocol);
  ctx.show("telegram_bits", telegram_bits);
  ctx.show("telegram_pulsewidth", telegram_pulsewidth);
  if (tristate != "") ctx.show("telegram_tristate", tristate);
  ctx.show("telegram_binary", _int2bin(telegram_binary, telegram_bits));
}

#ifdef IOTSA_WITH_WEB
void Iotsa433Telegram::formHandler_row(IotsaWebContext& ctx) {
  String tristate, brand, group, appliance, state;
  decode(tristate, brand, group, appliance, state);
  ctx.showCell("Seconds ago", String((millis() - receivedAt)/1000));
  ctx.showCell("Forwarding", statusName(status));
  ctx.showCell("Brand", brand);
  ctx.showCell("Group", group);
  ctx.showCell("Appliance", appliance);
  ctx.showCell("State", state);
  ctx.showCell("Protocol", String(telegram_protocol));
  ctx.showCell("Pulse width", String(telegram_pulsewidth));
  ctx.showCell("Bits", String(telegram_bits));
  ctx.showCell("Binary", _int2bin(telegram_binary, telegram_bits));
  ctx.showCell("Tristate", tristate);
}
#endif
