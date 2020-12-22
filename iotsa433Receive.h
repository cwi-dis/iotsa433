#ifndef _IOTSA433RECEIVE_H_
#define _IOTSA433RECEIVE_H_
#include "iotsa.h"
#include "iotsaApi.h"
#include "iotsaConfigFile.h"

#ifdef IOTSA_WITH_API
#define Iotsa433ReceiveModBaseMod IotsaApiMod
#else
#define Iotsa433ReceiveModBaseMod IotsaMod
#endif

class forwarder433 {
public:
  void configLoad(IotsaConfigFileLoad& cf, String& name);
  void configSave(IotsaConfigFileSave& cf, String& name);
  String url; // URL to send a request to
  String tristate; // if non-empty, only apply to this tri-state code
  String brand; // if non-empty, only apply to switches of this brand
  String dipswitches; // if non-empty only apply to switch with this dip-switch selection
  String button;  // if non-empty only apply to this button name
  String onoff; // if non-empty only apply to this on/off setting
  bool parameters;  // if True, add URL parameters for each parameter/value
};

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
  void _forward_one();
  void configLoad();
  void configSave();
  void handler();

  bool _addForwarder(forwarder433& newForwarder);
  bool _delForwarder(int index);
  bool _swapForwarder(int oldIndex, int newIndex);
  std::vector<forwarder433> forwarders;
};

#endif
