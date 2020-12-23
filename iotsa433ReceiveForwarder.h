#ifndef _IOTSA433RECEIVEFORWARDER_H_
#define _IOTSA433RECEIVEFORWARDER_H_
#include "iotsa.h"
#include "iotsaApi.h"
#include "iotsaConfigFile.h"

class Iotsa433ReveiveForwarder {
public:
  void configLoad(IotsaConfigFileLoad& cf, String& name);
  void configSave(IotsaConfigFileSave& cf, String& name);
  static void formHandlerTH(String& message); //< Emit <th> fields with names
  void formHandlerTD(String& message);  //< Emit <td> fields with data
  static void formHandler(String& message); //< Emit empty form to add forwarder
#ifdef IOTSA_WITH_WEB
  bool formArgHandler(IotsaWebServer *server);
#endif
#ifdef IOTSA_WITH_API
  void getHandler(JsonObject& reply);
  bool putHandler(const JsonVariant& request);
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
