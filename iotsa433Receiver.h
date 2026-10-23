#ifndef _IOTSA433RECEIVER_H_
#define _IOTSA433RECEIVER_H_
#include "iotsa.h"
#include "iotsaApi.h"
#include <RCSwitch.h>
#include "iotsa433Forwarder.h"
#include "iotsa433Telegram.h"

typedef std::function<void(void)> Iotsa433ReceiverCallback;

//
// The 433 MHz receiver: keeps a log of the last telegrams received, and
// forwards each one to every forwarder that matches it. The page
// /433receiver shows the log and the forwarders; GET /api/433receiver
// returns both, PUT replaces the forwarders, POST adds one.
//
class Iotsa433ReceiverMod : public IotsaModule {
public:
  Iotsa433ReceiverMod(IotsaApplication &_app, RCSwitch &_rcswitch, int _pin);
  void setup() override;
  void lateSetup() override;
  void loop() override;
  String info() override;
  // Called after a telegram was forwarded, and after a forward failed.
  void setStatusCallbacks(Iotsa433ReceiverCallback _forwarded, Iotsa433ReceiverCallback _failed);
protected:
  void getHandler(IotsaApiContext& ctx) override;
  void putHandler(IotsaApiContext& ctx) override;
  void postHandler(IotsaApiContext& ctx) override;
  void configLoad() override;
  void configSave() override;
#ifdef IOTSA_WITH_WEB
  String webTitle() override { return "433 MHz receiver"; }
  void formHandler_args(IotsaWebContext& ctx) override;
  void formHandler_fields(IotsaWebContext& ctx) override;
#endif
  void _received(uint32_t value, int telegram_protocol, int telegram_bits, int telegram_pulsewidth);
  void _forwardOne();

  RCSwitch &rcswitch;
  int pin;
  std::vector<Iotsa433Forwarder> forwarders;
  Iotsa433ReceiverCallback forwardedCallback;
  Iotsa433ReceiverCallback failedCallback;

  // The log: a ring buffer of the last LOG_SIZE telegrams, oldest at
  // logOut. logForward is the oldest one not yet forwarded.
  static const int LOG_SIZE = 16;
  Iotsa433Telegram telegrams[LOG_SIZE];
  int logIn = 0;
  int logOut = 0;
  int logForward = 0;
  static int _next(int i) { return (i + 1) % LOG_SIZE; }
  int _logSize() const { return (logIn - logOut + LOG_SIZE) % LOG_SIZE; }
  Iotsa433Telegram& _logAt(int i) { return telegrams[(logOut + i) % LOG_SIZE]; }

#ifdef IOTSA_WITH_WEB
  // The log as a read-only web collection.
  class TelegramLog : public IotsaWebCollection {
  public:
    TelegramLog(Iotsa433ReceiverMod& mod)
    : IotsaWebCollection(0, IotsaWebCollection::Table, IotsaWebContext::Transient),
      _mod(mod)
    {}
    int size() override { return _mod._logSize(); }
    IotsaModObject* at(int i) override { return &_mod._logAt(i); }
    void withExample(std::function<void(IotsaModObject&)> fn) override { Iotsa433Telegram example; fn(example); }
  private:
    Iotsa433ReceiverMod& _mod;
  };
  TelegramLog webLog;
  IotsaWebVector<Iotsa433Forwarder> webForwarders;
#endif
};

#endif
