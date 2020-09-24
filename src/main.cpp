/*
const moduleParent = module.parent;
if (module !== process.mainModule || (moduleParent !== Module && moduleParent !== undefined && moduleParent !== null)) {
  dialog.showErrorBox('Error', 'This program has been changed by others.')
  app.quit()
}

const { app, dialog } = require('electron')
const Module = require('module')

function getKey () {
  return KEY
}

function decrypt (body) {
  // const iv = body.slice(0, 16)
  // const data = body.slice(16)
  // const clearEncoding = 'utf8'
  // const cipherEncoding = 'binary'
  // const chunks = []
  // const decipher = require('crypto').createDecipheriv('aes-256-cbc', getKey(), iv)
  // decipher.setAutoPadding(true)
  // chunks.push(decipher.update(data, cipherEncoding, clearEncoding))
  // chunks.push(decipher.final(clearEncoding))
  // const code = chunks.join('')
  // return code
  // [native code]
}

const oldCompile = Module.prototype._compile
Object.defineProperty(Module.prototype, '_compile', {
  enumerable: true,
  value: function (content, filename) {
    if (filename.indexOf('app.asar') !== -1) {
      return oldCompile.call(this, decrypt(Buffer.from(content, 'base64')), filename)
    }
    return oldCompile.call(this, content, filename)
  }
})

try {
  require('./main.js')(getKey())
} catch (err) {
  dialog.showErrorBox('Error', err.stack)
  app.quit()
}

*/

#include <cstdlib>
#include <cstring>
#include <vector>
#include <napi.h>
#include <unordered_map>
#include "script.h"

#include "aes/aes.hpp"

#define KEY_LENGTH 32

#define FN_MODULE_PROTOTYPE__COMPILE 0

static const char errmsg[] = "This program has been changed by others.";

typedef struct AddonData {
  std::unordered_map<int, Napi::ObjectReference> modules;
  std::unordered_map<int, Napi::FunctionReference> functions;
} AddonData;

static void _log(Napi::Env env, Napi::Value value) {
  Napi::Object console = env.Global().As<Napi::Object>().Get("console").As<Napi::Object>();
  Napi::Function log = console.Get("log").As<Napi::Function>();
  log.Call(console, { value });
}

static void _error(Napi::Env env, Napi::Value value) {
  Napi::Object console = env.Global().As<Napi::Object>().Get("console").As<Napi::Object>();
  Napi::Function error = console.Get("error").As<Napi::Function>();
  error.Call(console, { value });
}

static std::vector<unsigned char> _getKeyVector() {
  const unsigned char key[KEY_LENGTH] = {
#include "key.txt"
  };

  return std::vector<unsigned char>(key, key + KEY_LENGTH);
}

static Napi::Array _getKey(const Napi::Env& env) {
  std::vector<unsigned char> key = _getKeyVector();
  Napi::Array arrkey = Napi::Array::New(env, key.size());
  for (uint32_t i = 0; i < key.size(); i++) {
    arrkey.Set(i, key[i]);
  }
  return arrkey;
}

static void _deleteAddonData(napi_env env, void* data, void* hint) {
  (void) env;
  (void) hint;
  // _log(env, Napi::String::New(env, "delete"));
  delete static_cast<AddonData*>(data);
}

static int pkcs7cut(unsigned char *p, int plen) {
  unsigned char last = p[plen - 1];
  if (last > 0 && last <= 16) {
    for (int x = 2; x <= last; x++) {
      if (p[plen - x] != last) {
        return plen;
      }
    }
    return plen - last;
  }

  return plen;
}

static std::string aesdec(const std::vector<unsigned char>& data, const std::vector<unsigned char>& key, const std::vector<unsigned char>& iv) {
  size_t l = data.size();
  unsigned char* encrypt = new unsigned char[l];
  memcpy(encrypt, data.data(), l);

  struct AES_ctx ctx;
  AES_init_ctx_iv(&ctx, key.data(), iv.data());
  AES_CBC_decrypt_buffer(&ctx, encrypt, l);

  unsigned char* out = new unsigned char[l + 1];
  memcpy(out, encrypt, l);
  out[l] = 0;

  int realLength = pkcs7cut(out, l);
  out[realLength] = 0;

  delete[] encrypt;
  std::string res = (char*)out;
  delete[] out;
  return res;
}

static std::vector<unsigned char> _jsBufferToCppBuffer(const Napi::Object& buf) {
  const char strlength[] = "length";
  int32_t len = buf.Get(strlength).As<Napi::Number>().Int32Value();
  std::vector<unsigned char> res(len);
  for (int32_t i = 0; i < len; i++) {
    res[i] = (unsigned char)(buf.Get(i).As<Napi::Number>().Int32Value());
  }
  return res;
}

static Napi::String _base64toCode(const Napi::Env& env, const Napi::String& base64) {
  Napi::Object Buffer = env.Global().Get("Buffer").As<Napi::Object>();
  Napi::Object body = Buffer.Get("from").As<Napi::Function>().Call(Buffer, { base64, Napi::String::New(env, "base64") }).As<Napi::Object>();

  Napi::Object iv = body.Get("slice").As<Napi::Function>().Call(body, { Napi::Number::New(env, 0), Napi::Number::New(env, 16) }).As<Napi::Object>();
  Napi::Object data = body.Get("slice").As<Napi::Function>().Call(body, { Napi::Number::New(env, 16) }).As<Napi::Object>();

  std::string _plainContent = aesdec(_jsBufferToCppBuffer(data), _getKeyVector(), _jsBufferToCppBuffer(iv));

  return Napi::String::New(env, _plainContent);
}

static Napi::Value modulePrototypeCompile(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  AddonData* addonData = static_cast<AddonData*>(info.Data());
  Napi::Object content = info[0].As<Napi::Object>();
  Napi::Object filename = info[1].As<Napi::Object>();
  Napi::Function oldCompile = addonData->functions[FN_MODULE_PROTOTYPE__COMPILE].Value();

  if (-1 != filename.Get("indexOf").As<Napi::Function>().Call(filename, { Napi::String::New(env, "app.asar") }).As<Napi::Number>().Int32Value()) {
    return oldCompile.Call(info.This(), { _base64toCode(env, content.As<Napi::String>()), filename });
  }
  return oldCompile.Call(info.This(), { content, filename });
}

static Napi::Function _makeRequireFunction(Napi::Env& env, const Napi::Object& module) {
  Napi::Function _makeRequire = env.RunScript(scriptRequire).As<Napi::Function>();
  return _makeRequire({ module }).As<Napi::Function>();
}

static Napi::Value _getModuleObject(Napi::Env& env, const Napi::Object& mainModule, const Napi::Object& thisExports) {
  Napi::Function _findFunction = env.RunScript(scriptFind).As<Napi::Function>();
  Napi::Value res = _findFunction({ mainModule, thisExports });
  if (res.IsNull()) {
    Napi::Error::New(env, "Cannot find module object.").ThrowAsJavaScriptException();
  }
  return res;
}

static void _showErrorAndQuit(const Napi::Env& env, const Napi::Object& electron, const Napi::String& message) {
  Napi::Value ELECTRON_RUN_AS_NODE = env.Global().As<Napi::Object>().Get("process").As<Napi::Object>().Get("env").As<Napi::Object>().Get("ELECTRON_RUN_AS_NODE");

  if (!ELECTRON_RUN_AS_NODE.IsUndefined() && ELECTRON_RUN_AS_NODE != Napi::Number::New(env, 0) && ELECTRON_RUN_AS_NODE != Napi::String::New(env, "")) {
    _error(env, message);
    exit(1);
  } else {
    Napi::Object dialog = electron.Get("dialog").As<Napi::Object>();
    dialog.Get("showErrorBox").As<Napi::Function>().Call(dialog, { Napi::String::New(env, "Error"), message });

    Napi::Object app = electron.Get("app").As<Napi::Object>();
    Napi::Function quit = app.Get("quit").As<Napi::Function>();
    quit.Call(app, {});
  }
}

static Napi::Object _init(Napi::Env env, Napi::Object exports) {
#ifdef _TARGET_ELECTRON_RENDERER_
  Napi::Object mainModule = env.Global().Get("module").As<Napi::Object>();
#else
  Napi::Object mainModule = env.Global().Get("process").As<Napi::Object>().Get("mainModule").As<Napi::Object>();
#endif

  Napi::Object module = _getModuleObject(env, mainModule, exports).As<Napi::Object>();
  Napi::Function require = _makeRequireFunction(env, module);

  Napi::Object electron = require({ Napi::String::New(env, "electron") }).As<Napi::Object>();
  Napi::Object Module = require({ Napi::String::New(env, "module") }).As<Napi::Object>();

  Napi::Value moduleParent = module.Get("parent");

#ifdef _TARGET_ELECTRON_RENDERER_
  if (moduleParent != mainModule) {
    Napi::Object ipcRenderer = electron.Get("ipcRenderer").As<Napi::Object>();
    Napi::Function sendSync = ipcRenderer.Get("sendSync").As<Napi::Function>();
    sendSync.Call(ipcRenderer, { Napi::String::New(env, "__SHOW_ERROR_AND_QUIT__") });
    return exports;
  }
#else
  if (module != mainModule || (moduleParent != Module && moduleParent != env.Undefined() && moduleParent != env.Null())) {
    _showErrorAndQuit(env, electron, Napi::String::New(env, errmsg));
    return exports;
  }
#endif

  AddonData* addonData = new AddonData;
  NAPI_THROW_IF_FAILED(env,
    napi_wrap(env, exports, addonData, _deleteAddonData, nullptr, nullptr),
    exports);

  Napi::Object ModulePrototype = Module.Get("prototype").As<Napi::Object>();
  addonData->functions[FN_MODULE_PROTOTYPE__COMPILE] = Napi::Persistent(ModulePrototype.Get("_compile").As<Napi::Function>());

  ModulePrototype.DefineProperty(Napi::PropertyDescriptor::Function(env, Napi::Object::New(env), "_compile", modulePrototypeCompile, napi_enumerable, addonData));

#ifdef _TARGET_ELECTRON_RENDERER_
  return exports;
#else

  Napi::Value ELECTRON_RUN_AS_NODE = env.Global().As<Napi::Object>().Get("process").As<Napi::Object>().Get("env").As<Napi::Object>().Get("ELECTRON_RUN_AS_NODE");

  if (ELECTRON_RUN_AS_NODE.IsUndefined() || ELECTRON_RUN_AS_NODE == Napi::Number::New(env, 0) || ELECTRON_RUN_AS_NODE == Napi::String::New(env, "")) {
    Napi::Object ipcMain = electron.Get("ipcMain").As<Napi::Object>();
    Napi::Function once = ipcMain.Get("once").As<Napi::Function>();

    once.Call(ipcMain, { Napi::String::New(env, "__SHOW_ERROR_AND_QUIT__"), Napi::Function::New(env, [](const Napi::CallbackInfo& info) -> Napi::Value {
      Napi::Env env = info.Env();
      Napi::Object event = info[0].As<Napi::Object>();
      Napi::Object mm = env.Global().Get("process").As<Napi::Object>().Get("mainModule").As<Napi::Object>();
      Napi::Function req = mm.Get("require").As<Napi::Function>();
      _showErrorAndQuit(env, req.Call(mm, { Napi::String::New(env, "electron") }).As<Napi::Object>(), Napi::String::New(env, errmsg));
      event.Set("returnValue", env.Null());
      return env.Undefined();
    }) });
  }

  try {
    require({ Napi::String::New(env, "./main.js") }).As<Napi::Function>().Call({ _getKey(env) });
  } catch (const Napi::Error& e) {
    _showErrorAndQuit(env, electron, e.Get("stack").As<Napi::String>());
  }
  return exports;
#endif
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, _init)
