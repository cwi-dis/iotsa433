#include "iotsa433Telegram.h"
#include "decode433.h"

static String _int2bin(int value, int telegram_bits) {
  String rv = String(value, BIN);
  while (rv.length() != (unsigned)telegram_bits) rv = "0" + rv;
  return rv;
}

bool Iotsa433Telegram::configLoad(IotsaConfigFileLoad& cf, const String& name) { 
    return false; 
}

void Iotsa433Telegram::configSave(IotsaConfigFileSave& cf, const String& name) {
}

#ifdef IOTSA_WITH_WEB
void Iotsa433Telegram::formHandler_TH(String& message, bool includeConfig) {
  message += "<th>seconds ago</th><th>status</th><th>brand</th><th>group</th><th>appliance</th><th>state</th><th>telegram_protocol</th><th>telegram_pulsewidth</th><th>telegram_bits</th><th>telegram_binary</th><th>telegram_tristate</th></tr>";
}

void Iotsa433Telegram::formHandler_TD(String& message, bool includeConfig) {
  message += "<td>"  + String((::millis()-millis)/1000.0) + "</td>";
  message += "<td>" + String(status) + "</td>";
  String group_buf;
  String appliance_buf;
  String state_buf;
#ifdef WITH_HEMA
  if (decode433_hema(telegram_binary, telegram_bits, group_buf, appliance_buf, state_buf)) {
      message += "<td>HEMA</td>";
      message += "</td><td>";
      message += group_buf;
      message += "</td><td>";
      message += appliance_buf;
      message += "</td><td>";
      message += state_buf;
      message += "</td>";
  } else 
#endif
#ifdef WITH_ELRO_FLAMINGO
  if (decode433_elro(telegram_binary, telegram_bits, group_buf, appliance_buf, state_buf)) {
      message += "<td>ELRO</td>";
      message += "</td><td>";
      message += group_buf;
      message += "</td><td>";
      message += appliance_buf;
      message += "</td><td>";
      message += state_buf;
      message += "</td>";
  } else 
#endif
  {
      message += "<td>unknown</td>";
      message += "</td><td>";
      message += "</td><td>";
      message += "</td><td>";
      message += "</td>";
  }
  // Low-level fields
  message += "<td>" + String(telegram_protocol) + "</td>";
  message += "<td>" + String(telegram_pulsewidth) + "</td>";
  message += "<td>" + String(telegram_bits) + "</td>";
  message += "<td>" + _int2bin(telegram_binary, telegram_bits) + "</td>";
  String tri_buf;
  (void)decode433_tristate(telegram_binary, telegram_bits, tri_buf);
  message += "<td>" + tri_buf + "</td>";
}

void Iotsa433Telegram::formHandler_fields(String& message, const String& text, const String& f_name, bool includeConfig) {
}

void Iotsa433Telegram::formHandler_emptyfields(String& message) {
}

bool Iotsa433Telegram::formHandler_args(IotsaWebServer *server, const String& f_name, bool includeConfig) {
    return false;
}
#endif
#ifdef IOTSA_WITH_API
void Iotsa433Telegram::getHandler(JsonObject& fRv) {
    
    fRv["time"] = (::millis() - millis)/1000.0;
    fRv["status"] = status;
    String group_buf;
    String appliance_buf;
    String state_buf;
#ifdef WITH_HEMA
    bool is_hema = decode433_hema(telegram_binary, 24, group_buf, appliance_buf, state_buf);
    if (is_hema) {
      fRv["brand"] = "HEMA";
      fRv["group"] = group_buf;
      fRv["appliance"] = appliance_buf;
      fRv["state"] = state_buf;
    }
#endif
#ifdef WITH_ELRO_FLAMINGO
    bool is_elro = decode433_elro(telegram_binary, 28, group_buf, appliance_buf, state_buf);
    if (is_elro) {
      fRv["brand"] = "ELRO";
      fRv["group"] = group_buf;
      fRv["appliance"] = appliance_buf;
      fRv["state"] = state_buf;
    }
#endif
    fRv["telegram_protocol"] = telegram_protocol;
    fRv["telegram_bits"] = telegram_bits;
    fRv["telegram_pulsewidth"] = telegram_pulsewidth;
    String tri_buf;
    if (decode433_tristate(telegram_binary, telegram_bits, tri_buf)) fRv["telegram_tristate"] = tri_buf;
    fRv["telegram_binary"] = _int2bin(telegram_binary, telegram_bits);
}

bool Iotsa433Telegram::putHandler(const JsonVariant& request) {
    return false;
}

bool Iotsa433Telegram::_parse(String& telegram_tristate, String& brand, String& group, String& appliance, String& state) {
  bool ok = decode433_tristate(telegram_binary, telegram_bits, telegram_tristate);
#ifdef WITH_HEMA
  if( decode433_hema(telegram_binary, telegram_bits, group, appliance, state)) {
    brand = "HEMA";
    ok = true;
  }
#endif
#ifdef WITH_ELRO_FLAMINGO
  if( decode433_elro(telegram_binary, telegram_bits, group, appliance, state)) {
    brand = "ELRO";
    ok = true;
  }
#endif
    return ok;
}
#endif
