#ifndef _IOTSA433RECEIVE_H_
#define _IOTSA433RECEIVE_H_
#include "iotsa.h"
#include "iotsaApi.h"

#ifdef IOTSA_WITH_API
#define Iotsa433ReceiveModBaseMod IotsaApiMod
#else
#define Iotsa433ReceiveModBaseMod IotsaMod
#endif

class Iotsa433ReceiveMod : public Iotsa433ReceiveModBaseMod {
public:
  using Iotsa433ReceiveModBaseMod::Iotsa433ReceiveModBaseMod;
  void setup();
  void serverSetup();
  void loop();
  String info();
protected:
  bool getHandler(const char *path, JsonObject& reply);
  bool putHandler(const char *path, const JsonVariant& request, JsonObject& reply);
  void _received(uint32_t value);
  void configLoad();
  void configSave();
  void handler();
  String argument;
};

#endif
