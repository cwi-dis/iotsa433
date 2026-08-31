#ifndef _IOTSA433SEND_H_
#define _IOTSA433SEND_H_
#include "iotsa.h"
#include "iotsaApi.h"

class Iotsa433SendMod : public IotsaModule {
public:
  using IotsaModule::IotsaModule;
  void setup() override;
  void lateSetup() override;
  void loop() override;
  String info() override;
protected:
  bool getHandler(const char *path, JsonObject& reply) override;
  bool putHandler(const char *path, const JsonVariant& request, JsonObject& reply) override;
  bool postHandler(const char *path, const JsonVariant& request, JsonObject& reply) override;
  void configLoad() override;
  void configSave() override;
  void webHandler() override;
  bool _send_set_protocol(int telegram_protocol, int telegram_pulsewidth);
  bool _send_binary(String telegram_binary);
  bool _send_tristate(String telegram_binary);
  bool _send_brand(int telegram_protocol, int telegram_pulsewidth, String brand, String group, String appliance, bool state);
};

#endif
