#ifndef _IOTSA433TELEGRAM_H_
#define _IOTSA433TELEGRAM_H_
#include "iotsa.h"
#include "iotsaApi.h"

//
// One received telegram, an entry in the receiver's log: shown as a row on
// the page and in GET /api/433receiver. It is not a setting, so it is never
// saved and can't be changed; the methods for that are empty.
//
class Iotsa433Telegram : public IotsaApiModObject {
public:
  bool configLoad(IotsaConfigFileLoad& cf, const String& name) override { return false; }
  void configSave(IotsaConfigFileSave& cf, const String& name) override {}
  void putHandler(IotsaApiContext& ctx) override { ctx.error(400, "received telegrams are read-only"); }
  void getHandler(IotsaApiContext& ctx) override;
#ifdef IOTSA_WITH_WEB
  void formHandler_row(IotsaWebContext& ctx) override;
#endif
  // What the telegram means: its tristate form (if it has one) and, if a
  // decoder recognises it, brand, group, appliance and state (brand is
  // "unknown" otherwise). False if neither.
  bool decode(String& tristate, String& brand, String& group, String& appliance, String& state);

  enum Status { queued, ignored, sent, failed };
  static const char *statusName(Status s);

  uint32_t receivedAt = 0;  // millis()
  uint32_t telegram_binary = 0;
  int telegram_protocol = 0;
  int telegram_bits = 0;
  int telegram_pulsewidth = 0;
  Status status = queued;   // what forwarding did with it
};

#endif // _IOTSA433TELEGRAM_H_
