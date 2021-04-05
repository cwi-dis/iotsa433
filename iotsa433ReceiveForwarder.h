#ifndef _IOTSA433RECEIVEFORWARDER_H_
#define _IOTSA433RECEIVEFORWARDER_H_
#include "iotsa.h"
#include "iotsaApi.h"
#include "iotsaConfigFile.h"
#include "iotsaRequest.h"

class Iotsa433ReceiveForwarder : public IotsaRequest {
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
  bool matches(String& _tristate, String& _brand, String& _group, String& _appliance, String& _state);
  bool send(String& _tristate, String& _brand, String& _group, String& _appliance, String& _state);

public:
  String telegram_tristate; // if non-empty, only apply to this tri-state telegram_binary
  String brand; // if non-empty, only apply to switches of this brand
  String group; // if non-empty only apply to switch with this dip-switch selection
  String appliance;  // if non-empty only apply to this appliance name
  String state; // if non-empty only apply to this on/off setting
  bool parameters;  // if True, add URL parameters for each parameter/value
};

#endif
