#include "iotsa.h"
#include "iotsa433Forwarder.h"

bool Iotsa433Forwarder::configLoad(IotsaConfigFileLoad& cf, const String& f_name) {
  bool ok = IotsaRequest::configLoad(cf, f_name);
  if (!ok) return false;
  cf.get(f_name + ".telegram_tristate", telegram_tristate, "");
  cf.get(f_name + ".brand", brand, "");
  cf.get(f_name + ".group", group, "");
  cf.get(f_name + ".appliance", appliance, "");
  cf.get(f_name + ".state", state, "");
  cf.get(f_name + ".parameters", parameters, false);
  return true;
}

void Iotsa433Forwarder::configSave(IotsaConfigFileSave& cf, const String& f_name) {
  IotsaRequest::configSave(cf, f_name);
  cf.put(f_name + ".telegram_tristate", telegram_tristate);
  cf.put(f_name + ".brand", brand);
  cf.put(f_name + ".group", group);
  cf.put(f_name + ".appliance", appliance);
  cf.put(f_name + ".state", state);
  cf.put(f_name + ".parameters", (int)parameters);
}

void Iotsa433Forwarder::putHandler(IotsaApiContext& ctx) {
  IotsaRequest::putHandler(ctx);
  ctx.arg("telegram_tristate", telegram_tristate, IotsaApiContext::Saved);
  ctx.arg("brand", brand, IotsaApiContext::Saved);
  ctx.arg("group", group, IotsaApiContext::Saved);
  ctx.arg("appliance", appliance, IotsaApiContext::Saved);
  ctx.arg("state", state, IotsaApiContext::Saved);
  ctx.arg("parameters", parameters, IotsaApiContext::Saved);
}

void Iotsa433Forwarder::getHandler(IotsaApiContext& ctx) {
  // The base class leaves the read right at "never" for its secrets.
  IotsaRequest::getHandler(ctx);
  ctx.showsNeedRight(IOTSA_RIGHT_NONE);
  ctx.show("telegram_tristate", telegram_tristate);
  ctx.show("brand", brand);
  ctx.show("group", group);
  ctx.show("appliance", appliance);
  ctx.show("state", state);
  ctx.show("parameters", parameters);
}

#ifdef IOTSA_WITH_WEB
void Iotsa433Forwarder::formHandler_args(IotsaWebContext& ctx) {
  IotsaRequest::formHandler_args(ctx);
  ctx.arg("telegram_tristate", telegram_tristate, IotsaWebContext::Saved);
  ctx.arg("brand", brand, IotsaWebContext::Saved);
  ctx.arg("group", group, IotsaWebContext::Saved);
  ctx.arg("appliance", appliance, IotsaWebContext::Saved);
  ctx.arg("state", state, IotsaWebContext::Saved);
  ctx.argCheckbox("parameters", parameters, IotsaWebContext::Saved);
}

// The fields for adding a new forwarder.
void Iotsa433Forwarder::formHandler_fields(IotsaWebContext& ctx) {
  IotsaRequest::formHandler_fields(ctx);
  ctx.showField("Only tristate telegram", "telegram_tristate", telegram_tristate, IotsaWebContext::Saved, "empty: any");
  ctx.showField("Only brand", "brand", brand, IotsaWebContext::Saved, "empty: any");
  ctx.showField("Only group", "group", group, IotsaWebContext::Saved, "empty: any");
  ctx.showField("Only appliance", "appliance", appliance, IotsaWebContext::Saved, "empty: any");
  ctx.showField("Only state", "state", state, IotsaWebContext::Saved, "empty: any");
  ctx.showCheckbox("Add the telegram to the URL", "parameters", parameters, IotsaWebContext::Saved, "as /433send takes it");
}

void Iotsa433Forwarder::formHandler_row(IotsaWebContext& ctx) {
  IotsaRequest::formHandler_row(ctx);
  ctx.showCell("Tristate", telegram_tristate);
  ctx.showCell("Brand", brand);
  ctx.showCell("Group", group);
  ctx.showCell("Appliance", appliance);
  ctx.showCell("State", state);
  ctx.showCell("Add telegram", parameters ? "yes" : "no");
}
#endif // IOTSA_WITH_WEB

bool Iotsa433Forwarder::matches(const String& _tristate, const String& _brand, const String& _group, const String& _appliance, const String& _state) const {
  if (telegram_tristate != "" && _tristate != telegram_tristate) return false;
  if (brand != "" && _brand != brand) return false;
  if (group != "" && _group != group) return false;
  if (appliance != "" && _appliance != appliance) return false;
  if (state != "" && _state != state) return false;
  return true;
}

bool Iotsa433Forwarder::send(const String& _tristate, const String& _brand, const String& _group, const String& _appliance, const String& _state) {
  String query;
  if (parameters) {
    // The fields as the /433send trigger of another iotsa433 takes them.
    if (_brand != "unknown" && _group != "") {
      query = "brand=" + _brand + "&group=" + _group + "&appliance=" + _appliance + "&state=" + _state;
    } else {
      query = "telegram_tristate=" + _tristate;
    } // xxxjack should send binary if triState unavailable
  }
  IFDEBUG IotsaSerial.printf("433recv: GET %s %s\n", url.c_str(), query.c_str());
  return IotsaRequest::send(query.c_str());
}
