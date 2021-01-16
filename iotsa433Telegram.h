#ifndef _IOTSA433TELEGRAM_H_
#define _IOTSA433TELEGRAM_H_
#include "iotsa.h"
#include "iotsaApi.h"


class Iotsa433Telegram : public IotsaApiModObject {
public:
  bool configLoad(IotsaConfigFileLoad& cf, String& name) override;
  void configSave(IotsaConfigFileSave& cf, String& name) override;
#ifdef IOTSA_WITH_WEB
  static void formHandlerTH(String& message) /*override*/; //< Emit <th> fields with names
  static void formHandler(String& message) /*override*/; //< Emit empty form to add forwarder
  void formHandler(String& message, String& text, String& f_name) override;
  void formHandlerTD(String& message) override;  //< Emit <td> fields with data
  bool formArgHandler(IotsaWebServer *server, String f_name) override;
#endif
#ifdef IOTSA_WITH_API
  void getHandler(JsonObject& reply) override;
  bool putHandler(const JsonVariant& request) override;
#endif
public:
  bool _parse(String& telegram_tristate, String& brand, String& group, String& appliance, String& state);
public:
  uint32_t millis;
  uint32_t code;
  int telegram_protocol;
  int telegram_bits;
  int telegram_pulsewidth;
};

#endif // _IOTSA433TELEGRAM_H_