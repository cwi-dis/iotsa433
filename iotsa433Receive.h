#ifndef _IOTSA433RECEIVE_H_
#define _IOTSA433RECEIVE_H_
#include "iotsa.h"
#include "iotsaApi.h"
#include "iotsa433ReceiveForwarder.h"

#ifdef IOTSA_WITH_API
#define Iotsa433ReceiveModBaseMod IotsaApiMod
#else
#define Iotsa433ReceiveModBaseMod IotsaMod
#endif

typedef std::function<void(void)> Iotsa433ReceiveCallback;

class Iotsa433ReceiveMod : public Iotsa433ReceiveModBaseMod {
public:
  using Iotsa433ReceiveModBaseMod::Iotsa433ReceiveModBaseMod;
  void setup() override;
  void serverSetup() override;
  void loop() override;
  String info() override;
  void setStatusCallbacks(Iotsa433ReceiveCallback _ok, Iotsa433ReceiveCallback _notok);
protected:
  bool getHandler(const char *path, JsonObject& reply) override;
  bool putHandler(const char *path, const JsonVariant& request, JsonObject& reply) override;
  bool postHandler(const char *path, const JsonVariant& request, JsonObject& reply) override;
  void _received(uint32_t value, int telegram_protocol, int telegram_bits, int telegram_pulsewidth);
  void _forward_one();
  void configLoad() override;
  void configSave() override;
  void handler();

  bool _addForwarder(Iotsa433ReceiveForwarder& newForwarder);
  bool _delForwarder(int index);
  bool _swapForwarder(int oldIndex, int newIndex);
  std::vector<Iotsa433ReceiveForwarder> forwarders;
  Iotsa433ReceiveCallback statusOkCallback;
  Iotsa433ReceiveCallback statusNotOkCallback;
};

#endif
