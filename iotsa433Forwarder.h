#ifndef _IOTSA433FORWARDER_H_
#define _IOTSA433FORWARDER_H_
#include "iotsa.h"
#include "iotsaApi.h"
#include "iotsaConfigFile.h"
#include "iotsaRequest.h"

//
// A forwarder: a filter on received telegrams, and the request (URL,
// credentials) to send when one matches. An empty filter field matches
// anything. Every matching forwarder fires, in list order
// (cwi-dis/iotsa433#2).
//
class Iotsa433Forwarder : public IotsaRequest {
public:
  bool configLoad(IotsaConfigFileLoad& cf, const String& name) override;
  void configSave(IotsaConfigFileSave& cf, const String& name) override;
  void putHandler(IotsaApiContext& ctx) override;
  void getHandler(IotsaApiContext& ctx) override;
#ifdef IOTSA_WITH_WEB
  void formHandler_args(IotsaWebContext& ctx) override;
  void formHandler_fields(IotsaWebContext& ctx) override;
  void formHandler_row(IotsaWebContext& ctx) override;
#endif
  bool matches(const String& _tristate, const String& _brand, const String& _group, const String& _appliance, const String& _state) const;
  bool send(const String& _tristate, const String& _brand, const String& _group, const String& _appliance, const String& _state);

  String telegram_tristate; // only this tristate telegram
  String brand;             // only this brand
  String group;             // only this group (DIP switches)
  String appliance;         // only this appliance
  String state;             // only this state (on, off)
  bool parameters = false;  // add the telegram's fields to the URL, as /433send takes them
};

#endif
