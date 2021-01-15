#include "iotsa433Telegram.h"
#include "decode433.h"

static String _int2bin(int value, int bits) {
  String rv = String(value, BIN);
  while (rv.length() != (unsigned)bits) rv = "0" + rv;
  return rv;
}

bool Iotsa433Telegram::configLoad(IotsaConfigFileLoad& cf, String& name) { 
    return false; 
}

void Iotsa433Telegram::configSave(IotsaConfigFileSave& cf, String& name) {
}

#ifdef IOTSA_WITH_WEB
void Iotsa433Telegram::formHandlerTH(String& message) {
  message += "<th>seconds ago</th><th>decimal</th><th>binary</th><th>protocol</th><th>bits</th><th>bitTime</th><th>tristate</th><th>brand</th><th>dip switches</th><th>button</th><th>on/off</th></tr>";
}

void Iotsa433Telegram::formHandlerTD(String& message) {
    message += "<td>"  + String((::millis()-millis)/1000.0) + "</td>";
    message += "<td>" + String(code) + "</td>";
    message += "<td>" + _int2bin(code, bits) + "</td>";
    message += "<td>" + String(protocol) + "</td>";
    message += "<td>" + String(bits) + "</td>";
    message += "<td>" + String(bitTime) + "</td>";
    message += "<td>";
    String tri_buf;
    if (decode433_tristate(code, bits, tri_buf)) {
        message += "<code>" + tri_buf + "</code>";
    } else {
        message += "not decodable";
    }
    message += "</td>";
    String dip_buf;
    String button_buf;
    String onoff_buf;
    #ifdef WITH_HEMA
    if (decode433_hema(code, bits, dip_buf, button_buf, onoff_buf)) {
        message += "<td>HEMA</td>";
        message += "</td><td>";
        message += dip_buf;
        message += "</td><td>";
        message += button_buf;
        message += "</td><td>";
        message += onoff_buf;
        message += "</td></tr>";
    } else 
    #endif
    #ifdef WITH_ELRO_FLAMINGO
    if (decode433_elro(code, bits, dip_buf, button_buf, onoff_buf)) {
        message += "<td>ELRO</td>";
        message += "</td><td>";
        message += dip_buf;
        message += "</td><td>";
        message += button_buf;
        message += "</td><td>";
        message += onoff_buf;
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
    fRv["protocol"] = protocol;
    fRv["bits"] = bits;
    fRv["bitTime"] = bitTime;
    String tri_buf;
    if (decode433_tristate(code, bits, tri_buf)) fRv["tristate"] = tri_buf;
    fRv["binary"] = _int2bin(code, bits);
    String dip_buf;
    String button_buf;
    String onoff_buf;
#ifdef WITH_HEMA
    bool is_hema = decode433_hema(code, 24, dip_buf, button_buf, onoff_buf);
    if (is_hema) {
      fRv["brand"] = "HEMA";
      fRv["dipswitches"] = dip_buf;
      fRv["button"] = button_buf;
      fRv["onoff"] = onoff_buf;
    }
#endif
#ifdef WITH_ELRO_FLAMINGO
    bool is_elro = decode433_elro(code, 24, dip_buf, button_buf, onoff_buf);
    if (is_elro) {
      fRv["brand"] = "ELRO";
      fRv["dipswitches"] = dip_buf;
      fRv["button"] = button_buf;
      fRv["onoff"] = onoff_buf;
    }
#endif
}
bool Iotsa433Telegram::putHandler(const JsonVariant& request) {
    return false;
}

bool Iotsa433Telegram::_parse(String& tristate, String& brand, String& dipswitches, String& button, String& onoff) {
  bool ok = decode433_tristate(code, bits, tristate);
#ifdef WITH_HEMA
  if( decode433_hema(code, bits, dipswitches, button, onoff)) {
    brand = "HEMA";
    ok = true;
  }
#endif
#ifdef WITH_ELRO_FLAMINGO
  if( decode433_elro(code, bits, dipswitches, button, onoff)) {
    brand = "ELRO";
    ok = true;
  }
#endif
    return ok;
}
#endif
