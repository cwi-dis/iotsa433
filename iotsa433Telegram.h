#ifndef _IOTSA433TELEGRAM_H_
#define _IOTSA433TELEGRAM_H_
#include "iotsa.h"
#include "iotsaApi.h"


class Iotsa433Telegram : public IotsaApiModObject {
public:
  bool configLoad(IotsaConfigFileLoad& cf, const String& name) override;
  void configSave(IotsaConfigFileSave& cf, const String& name) override;
#ifdef IOTSA_WITH_WEB
  static void formHandler_TH(String& message, bool includeConfig) /*override*/; //< Emit <th> fields with names
  static void formHandler_emptyfields(String& message) /*override*/; //< Emit empty form to add forwarder
  void formHandler_fields(String& message, const String& text, const String& f_name, bool includeConfig) override;
  void formHandler_TD(String& message, bool includeConfig) override;  //< Emit <td> fields with data
  bool formHandler_args(IotsaWebServer *server, const String& f_name, bool includeConfig) override;
#endif
#ifdef IOTSA_WITH_API
  void getHandler(JsonObject& reply) override;
  bool putHandler(const JsonVariant& request) override;
#endif
public:
  bool _parse(String& telegram_tristate, String& brand, String& group, String& appliance, String& state);
public:
  uint32_t millis;
  uint32_t telegram_binary;
  int telegram_protocol;
  int telegram_bits;
  int telegram_pulsewidth;
  enum TelegramStatus { queued, ignored, sending, sent_ok, sent_fail} status; // Forwarding status

};

#endif // _IOTSA433TELEGRAM_H_