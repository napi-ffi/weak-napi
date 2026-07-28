#include <napi.h>
#include <setimmediate.h>

#if NAPI_VERSION < 8
#error "weak-napi requires NAPI_VERSION >= 8 (napi_type_tag_object support), \
which is used to safely validate ObjectInfo instances passed to WeakTag"
#endif

using namespace Napi;

namespace {

// Identifies JS objects that were actually constructed as ObjectInfo, so
// WeakTag can refuse to Unwrap() objects of any other type (see GH #1).
constexpr napi_type_tag kObjectInfoTypeTag = {
  0x8f6f6a6f5a3b4c1aULL, 0x9d2e7c9b1f4a6e05ULL
};

class ObjectInfo : public ObjectWrap<ObjectInfo> {
 public:
  ObjectInfo(const CallbackInfo& args) : ObjectWrap(args) {
    if (!args[0].IsObject() && !args[0].IsFunction())
      throw Error::New(Env(), "target should be object");
    if (!args[1].IsFunction())
      throw Error::New(Env(), "callback should be function");
    Value().TypeTag(&kObjectInfoTypeTag);
    Ref();
    target_.Reset(args[0].As<Object>(), 0);
    callback_.Reset(args[1].As<Function>(), 1);
  }

  void OnFree() {
    SetImmediate(Env(), [this]() {
      callback_.MakeCallback(Value(), {});
      callback_.Reset();
      Reset();
    });
  }

  Napi::Value GetTarget(const CallbackInfo&) {
    return target_.IsEmpty() ? Env().Undefined() : target_.Value();
  }

  static Function GetClass(Napi::Env env) {
    return DefineClass(env, "ObjectInfo", {
      ObjectInfo::InstanceAccessor("target", &ObjectInfo::GetTarget, nullptr),
    });
  }

  ObjectReference target_;
  FunctionReference callback_;
};

class WeakTag : public ObjectWrap<WeakTag> {
 public:
  WeakTag(const CallbackInfo& args) : ObjectWrap(args) {
    if (args[0].IsObject()) {
      Object obj = args[0].As<Object>();
      if (obj.CheckTypeTag(&kObjectInfoTypeTag))
        info_ = ObjectInfo::Unwrap(obj);
    }
    if (info_ == nullptr)
      throw Error::New(Env(), "First argument needs to be ObjectInfo");
  }

  ~WeakTag() {
    if (info_ != nullptr)
      info_->OnFree();
  }

  static Function GetClass(Napi::Env env) {
    return DefineClass(env, "WeakTag", {});
  }

  ObjectInfo* info_ = nullptr;
};

Object Init(Env env, Object exports) {
  exports["WeakTag"] = WeakTag::GetClass(env);
  exports["ObjectInfo"] = ObjectInfo::GetClass(env);
  return exports;
}

} // anonymous namespace

NODE_API_MODULE(weakref, Init)
