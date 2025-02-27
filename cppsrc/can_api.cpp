#include <stdio.h>
#include <napi.h>
#include <thread>
#include <iostream>
#include "can_api.h"

Napi::FunctionReference CanApi::constructor;

bool CanApi::CheckCanStatus(char* label, canStatus status) {
  switch(status) {
    case canOK:
      return true;
      break;
    case canERR_NOMSG:
      return false;
      break;
    default:
      printf("ERROR %s() FAILED, Err= %d <line: %d>\n", label, status, __LINE__);
      return false;
      break;
  }
}

Napi::Object CanApi::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func = DefineClass(env, "CanApi", {
    InstanceMethod("readMessage", &CanApi::ReadMessage),
    InstanceMethod("sendMessage", &CanApi::SendMessage),
    InstanceMethod("open", &CanApi::Open),
    InstanceMethod("close", &CanApi::Close),
  });

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();

  exports.Set("CanApi", func);

  return exports;
}

CanApi::CanApi(const Napi::CallbackInfo& info): Napi::ObjectWrap<CanApi>(info) {
  Napi::Env env = info.Env();
  Napi::HandleScope scope(env);

  Napi::Object bitRates = Napi::Object::New(env);
  bitRates.Set(Napi::String::New(env, "canBITRATE_1M"), canBITRATE_1M);
  bitRates.Set(Napi::String::New(env, "canBITRATE_500K"), canBITRATE_500K);
  bitRates.Set(Napi::String::New(env, "canBITRATE_250K"), canBITRATE_250K);
  bitRates.Set(Napi::String::New(env, "canBITRATE_125K"), canBITRATE_125K);
  bitRates.Set(Napi::String::New(env, "canBITRATE_100K"), canBITRATE_100K);
  bitRates.Set(Napi::String::New(env, "canBITRATE_83K"), canBITRATE_83K);
  bitRates.Set(Napi::String::New(env, "canBITRATE_62K"), canBITRATE_62K);
  bitRates.Set(Napi::String::New(env, "canBITRATE_50K"), canBITRATE_50K);
  bitRates.Set(Napi::String::New(env, "canBITRATE_10K"), canBITRATE_10K);

  this->channel = (int)info[0].As<Napi::Number>();
  this->canBitRate = (int)bitRates.Get(info[1].As<Napi::String>()).As<Napi::Number>();

  canInitializeLibrary();
}

Napi::Value CanApi::ReadMessage(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  Napi::Object options = info[0].As<Napi::Object>();

  canStatus status; 
  long identifier;
  uint32_t dlc;
  uint32_t flags;
  uint8_t data[8];
  unsigned long time;

  uint32_t optionTimeout = options.Get("timeout").As<Napi::Number>().Uint32Value();

  if (optionTimeout > 0) {
    status = canReadWait(this->handle, &identifier, data, &dlc, &flags, &time, optionTimeout);
  } else {
    status = canRead(this->handle, &identifier, data, &dlc, &flags, &time);
  }

  bool messageOk = CanApi::CheckCanStatus("ReadMessage::canRead", status);

  if (!messageOk) {
    return Napi::Boolean::New(env, false);
  }

  Napi::Object output = Napi::Object::New(env);

  output.Set(Napi::String::New(env, "identifier"), Napi::Number::New(env, identifier));
  output.Set(Napi::String::New(env, "data"), Napi::Buffer<uint8_t>::Copy(env, data, dlc));
  output.Set(Napi::String::New(env, "dlc"), Napi::Number::New(env, dlc));

  return output;
}

Napi::Value CanApi::SendMessage(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  int32_t identifier = info[0].As<Napi::Number>();
  uint8_t* data = info[1].As<Napi::Buffer<uint8_t>>().Data();
  int32_t dlc = info[2].As<Napi::Number>();

  canStatus status = canWriteWait(this->handle, identifier, data, dlc, canMSG_EXT, 1000);
  bool sendOk = CanApi::CheckCanStatus("SendMessage::canWrite", status);

  return Napi::Boolean::New(env, sendOk);
}

Napi::Value CanApi::Open(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  canStatus status;

  this->handle = canOpenChannel(this->channel, canOPEN_ACCEPT_VIRTUAL);
  CanApi::CheckCanStatus("Constructor::canOpenChannel", (canStatus)this->handle);
  
  status = canSetBusParams(this->handle, this->canBitRate, 0, 0, 0, 0, 0);
  CanApi::CheckCanStatus("Constructor::canSetBusParams", status);

  status = canBusOn(this->handle);
  CanApi::CheckCanStatus("Constructor::canBusOn", status);

  this->opened = true;

  return Napi::Boolean::New(env, this->opened);
}

Napi::Value CanApi::Close(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  canStatus status;

  this->opened = false;

  status = canBusOff(this->handle);
  CanApi::CheckCanStatus("CanBusOff", status);
  status = canClose(this->handle);
  CanApi::CheckCanStatus("CanClose", status);

  return Napi::Boolean::New(env, this->opened);
}