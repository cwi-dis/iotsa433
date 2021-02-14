#ifndef _IOTSA433SEND_H_
#define _IOTSA433SEND_H_
#include "iotsa.h"
#include "iotsaApi.h"

#ifdef IOTSA_WITH_API
#define Iotsa433SendModBaseMod IotsaApiMod
#else
#define Iotsa433SendModBaseMod IotsaMod
#endif

typedef std::function<void(void)> callback;

class Iotsa433SendMod : public Iotsa433SendModBaseMod {
public:
  using Iotsa433SendModBaseMod::Iotsa433SendModBaseMod;
  void setup() override;
  void serverSetup() override;
  void loop() override;
  String info() override;
protected:
  bool getHandler(const char *path, JsonObject& reply) override;
  bool putHandler(const char *path, const JsonVariant& request, JsonObject& reply) override;
  bool postHandler(const char *path, const JsonVariant& request, JsonObject& reply) override;
  void configLoad() override;
  void configSave() override;
  void handler();
  bool _send_set_protocol(int telegram_protocol, int telegram_pulsewidth);
  bool _send_binary(String telegram_binary);
  bool _send_tristate(String telegram_binary);
  bool _send_brand(int telegram_protocol, int telegram_pulsewidth, String brand, String group, String appliance, bool state);
};

#endif
