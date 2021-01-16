#include "iotsa433Telegram.h"
#include "decode433.h"

static String _int2bin(int value, int telegram_bits) {
  String rv = String(value, BIN);
  while (rv.length() != (unsigned)telegram_bits) rv = "0" + rv;
  return rv;
}

bool Iotsa433Telegram::configLoad(IotsaConfigFileLoad& cf, String& name) { 
    return false; 
}

void Iotsa433Telegram::configSave(IotsaConfigFileSave& cf, String& name) {
}

#ifdef IOTSA_WITH_WEB
void Iotsa433Telegram::formHandlerTH(String& message) {
  message += "<th>seconds ago</th><th>decimal</th><th>telegram_binary</th><th>telegram_protocol</th><th>telegram_bits</th><th>telegram_pulsewidth</th><th>telegram_tristate</th><th>brand</th><th>dip switches</th><th>appliance</th><th>on/off</th></tr>";
}

void Iotsa433Telegram::formHandlerTD(String& message) {
    message += "<td>"  + String((::millis()-millis)/1000.0) + "</td>";
    message += "<td>" + String(code) + "</td>";
    message += "<td>" + _int2bin(code, telegram_bits) + "</td>";
    message += "<td>" + String(telegram_protocol) + "</td>";
    message += "<td>" + String(telegram_bits) + "</td>";
    message += "<td>" + String(telegram_pulsewidth) + "</td>";
    message += "<td>";
    String tri_buf;
    if (decode433_tristate(code, telegram_bits, tri_buf)) {
        message += "<code>" + tri_buf + "</code>";
    } else {
        message += "not decodable";
    }
    message += "</td>";
    String dip_buf;
    String appliance_buf;
    String state_buf;
    #ifdef WITH_HEMA
    if (decode433_hema(code, telegram_bits, dip_buf, appliance_buf, state_buf)) {
        message += "<td>HEMA</td>";
        message += "</td><td>";
        message += dip_buf;
        message += "</td><td>";
        message += appliance_buf;
        message += "</td><td>";
        message += state_buf;
        message += "</td></tr>";
    } else 
    #endif
    #ifdef WITH_ELRO_FLAMINGO
    if (decode433_elro(code, telegram_bits, dip_buf, appliance_buf, state_buf)) {
        message += "<td>ELRO</td>";
        message += "</td><td>";
        message += dip_buf;
        message += "</td><td>";
        message += appliance_buf;
        message += "</td><td>";
        message += state_buf;
        message += "</td></tr>";
    } else 
    #endif
    {
        message += "<td>unknown</td>";
        message += "</td><td>";
        message += "</td><td>";
        message += "</td><td>";
        message += "</td>";
    }
}

void Iotsa433Telegram::formHandler(String& message, String& text, String& f_name) {
}

void Iotsa433Telegram::formHandler(String& message) {
}

bool Iotsa433Telegram::formArgHandler(IotsaWebServer *server, String f_name) {
    return false;
}
#endif
#ifdef IOTSA_WITH_API
void Iotsa433Telegram::getHandler(JsonObject& fRv) {
    
    fRv["time"] = (::millis() - millis)/1000.0;
    fRv["telegram_protocol"] = telegram_protocol;
    fRv["telegram_bits"] = telegram_bits;
    fRv["telegram_pulsewidth"] = telegram_pulsewidth;
    String tri_buf;
    if (decode433_tristate(code, telegram_bits, tri_buf)) fRv["telegram_tristate"] = tri_buf;
    fRv["telegram_binary"] = _int2bin(code, telegram_bits);
    String dip_buf;
    String appliance_buf;
    String state_buf;
#ifdef WITH_HEMA
    bool is_hema = decode433_hema(code, 24, dip_buf, appliance_buf, state_buf);
    if (is_hema) {
      fRv["brand"] = "HEMA";
      fRv["group"] = dip_buf;
      fRv["appliance"] = appliance_buf;
      fRv["state"] = state_buf;
    }
#endif
#ifdef WITH_ELRO_FLAMINGO
    bool is_elro = decode433_elro(code, 24, dip_buf, appliance_buf, state_buf);
    if (is_elro) {
      fRv["brand"] = "ELRO";
      fRv["group"] = dip_buf;
      fRv["appliance"] = appliance_buf;
      fRv["state"] = state_buf;
    }
#endif
}
bool Iotsa433Telegram::putHandler(const JsonVariant& request) {
    return false;
}

bool Iotsa433Telegram::_parse(String& telegram_tristate, String& brand, String& group, String& appliance, String& state) {
  bool ok = decode433_tristate(code, telegram_bits, telegram_tristate);
#ifdef WITH_HEMA
  if( decode433_hema(code, telegram_bits, group, appliance, state)) {
    brand = "HEMA";
    ok = true;
  }
#endif
#ifdef WITH_ELRO_FLAMINGO
  if( decode433_elro(code, telegram_bits, group, appliance, state)) {
    brand = "ELRO";
    ok = true;
  }
#endif
    return ok;
}
#endif
