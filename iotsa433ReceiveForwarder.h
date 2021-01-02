#ifndef _IOTSA433RECEIVEFORWARDER_H_
#define _IOTSA433RECEIVEFORWARDER_H_
#include "iotsa.h"
#include "iotsaApi.h"
#include "iotsaConfigFile.h"

class Iotsa433ReveiveForwarder : public IotsaApiModObject {
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
  bool matches(String& _tristate, String& _brand, String& _dipswitches, String& _button, String& _onoff);
  bool send(String& _tristate, String& _brand, String& _dipswitches, String& _button, String& _onoff);

public:
  String url; // URL to send a request to
  String tristate; // if non-empty, only apply to this tri-state code
  String brand; // if non-empty, only apply to switches of this brand
  String dipswitches; // if non-empty only apply to switch with this dip-switch selection
  String button;  // if non-empty only apply to this button name
  String onoff; // if non-empty only apply to this on/off setting
  bool parameters;  // if True, add URL parameters for each parameter/value
};

#endif
