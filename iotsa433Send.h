#ifndef _IOTSA433SEND_H_
#define _IOTSA433SEND_H_
#include "iotsa.h"
#include "iotsaApi.h"

#ifdef IOTSA_WITH_API
#define Iotsa433SendModBaseMod IotsaApiMod
#else
#define Iotsa433SendModBaseMod IotsaMod
#endif

class Iotsa433SendMod : public Iotsa433SendModBaseMod {
public:
  using Iotsa433SendModBaseMod::Iotsa433SendModBaseMod;
  void setup();
  void serverSetup();
  void loop();
  String info();
protected:
  bool getHandler(const char *path, JsonObject& reply);
  bool putHandler(const char *path, const JsonVariant& request, JsonObject& reply);
  void configLoad();
  void configSave();
  void handler();
  String argument;
};

#endif
