/*

const { app, dialog } = require('electron')
const crypto = require('crypto')
const Module = require('module')

const moduleParent = module.parent;
if (module !== process.mainModule || (moduleParent !== Module && moduleParent !== undefined && moduleParent !== null)) {
  dialog.showErrorBox('Error', 'This program has been changed by others.')
  app.quit()
}

function getKey () {
  return KEY
}

function decrypt (body) {
  const iv = body.slice(0, 16)
  const data = body.slice(16)
  const clearEncoding = 'utf8'
  const cipherEncoding = 'binary'
  const chunks = []
  const decipher = crypto.createDecipheriv('aes-256-cbc', getKey(), iv)
  decipher.setAutoPadding(true)
  chunks.push(decipher.update(data, cipherEncoding, clearEncoding))
  chunks.push(decipher.final(clearEncoding))
  const code = chunks.join('')
  return code
}

const oldCompile = Module.prototype._compile

Module.prototype._compile = function (content, filename) {
  if (filename.indexOf('app.asar') !== -1) {
    return oldCompile.call(this, decrypt(Buffer.from(content, 'base64')), filename)
  }
  return oldCompile.call(this, content, filename)
}

try {
  require('./main.js')(getKey())
} catch (err) {
  dialog.showErrorBox('Error', err.stack)
  app.quit()
}

*/

#include <cstdlib>
#include <napi.h>
#include <unordered_map>
#include "script.h"

#define KEY_LENGTH 32

#define FN_MODULE_PROTOTYPE__COMPILE 0
#define FN_CRYPTO_CREATEDECIPHERIV 1

#define MD_CRYPTO 1

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

static Napi::Array _getKey(const Napi::Env& env) {
  const unsigned char key[KEY_LENGTH] = {
#include "key.txt"
  };
  Napi::Array arrkey = Napi::Array::New(env, KEY_LENGTH);
  for (uint32_t i = 0; i < KEY_LENGTH; i++) {
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

static Napi::Value _decrypt(const Napi::Env& env, const Napi::Object& body, AddonData* addonData) {
  Napi::Object iv = body.Get("slice").As<Napi::Function>().Call(body, { Napi::Number::New(env, 0), Napi::Number::New(env, 16) }).As<Napi::Object>();
  Napi::Object data = body.Get("slice").As<Napi::Function>().Call(body, { Napi::Number::New(env, 16) }).As<Napi::Object>();
  Napi::String clearEncoding = Napi::String::New(env, "utf8");
  Napi::String cipherEncoding = Napi::String::New(env, "binary");
  Napi::Array chunks = Napi::Array::New(env);
  Napi::Object crypto = addonData->modules[MD_CRYPTO].Value();
  Napi::Function createDecipheriv = addonData->functions[FN_CRYPTO_CREATEDECIPHERIV].Value();

  const char algorithm[12] = { 0x61, 0x65, 0x73, 0x2d, 0x32, 0x35, 0x36, 0x2d, 0x63, 0x62, 0x63, 0 }; // "aes-256-cbc"

  Napi::Object Buffer = env.Global().Get("Buffer").As<Napi::Object>();
  Napi::Object keybuf = Buffer.Get("from").As<Napi::Function>().Call(Buffer, { _getKey(env) }).As<Napi::Object>();

  Napi::Object decipher = createDecipheriv.Call(crypto, { Napi::String::New(env, algorithm, 11), keybuf, iv }).As<Napi::Object>();
  decipher.Get("setAutoPadding").As<Napi::Function>().Call(decipher, { Napi::Boolean::New(env, true) });
  chunks.Get("push").As<Napi::Function>().Call(chunks, { decipher.Get("update").As<Napi::Function>().Call(decipher, { data, cipherEncoding, clearEncoding }) });
  chunks.Get("push").As<Napi::Function>().Call(chunks, { decipher.Get("final").As<Napi::Function>().Call(decipher, { clearEncoding }) });
  Napi::String code = chunks.Get("join").As<Napi::Function>().Call(chunks, { Napi::String::New(env, "") }).As<Napi::String>();

  return code;
}

static Napi::Value modulePrototypeCompile(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  AddonData* addonData = static_cast<AddonData*>(info.Data());
  Napi::Object content = info[0].As<Napi::Object>();
  Napi::Object filename = info[1].As<Napi::Object>();
  Napi::Function oldCompile = addonData->functions[FN_MODULE_PROTOTYPE__COMPILE].Value();

  if (-1 != filename.Get("indexOf").As<Napi::Function>().Call(filename, { Napi::String::New(env, "app.asar") }).As<Napi::Number>().Int32Value()) {
    Napi::Object Buffer = env.Global().Get("Buffer").As<Napi::Object>();
    Napi::Object body = Buffer.Get("from").As<Napi::Function>().Call(Buffer, { content, Napi::String::New(env, "base64") }).As<Napi::Object>();
    
    return oldCompile.Call(info.This(), { _decrypt(env, body, addonData), filename });
  }
  return oldCompile.Call(info.This(), { content, filename });
}

static Napi::Value _runScript(const Napi::Env& env, const Napi::String& script) {
  napi_value res;
  NAPI_THROW_IF_FAILED(env, napi_run_script(env, script, &res), env.Undefined());
  return Napi::Value(env, res);
}

static Napi::Value _runScript(const Napi::Env& env, const std::string& script) {
  return _runScript(env, Napi::String::New(env, script));
}

static Napi::Value _runScript(const Napi::Env& env, const char* script) {
  return _runScript(env, Napi::String::New(env, script));
}

static Napi::Function _makeRequireFunction(const Napi::Env& env, const Napi::Object& module) {
  Napi::Function _makeRequire = _runScript(env, scriptRequire).As<Napi::Function>();
  return _makeRequire({ module }).As<Napi::Function>();
}

static Napi::Value _getModuleObject(const Napi::Env& env, const Napi::Object& exports) {
  Napi::Function _findFunction = _runScript(env, scriptFind).As<Napi::Function>();
  Napi::Value res = _findFunction({ exports });
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

static Napi::Value cryptoGetter(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  AddonData* addonData = static_cast<AddonData*>(info.Data());

  return addonData->functions[FN_CRYPTO_CREATEDECIPHERIV].Value();
}
static Napi::Value cryptoSetter(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  Napi::Error err = Napi::Error::New(env, errmsg);

  err.ThrowAsJavaScriptException();
  return env.Undefined();
}

static Napi::Object _init(Napi::Env env, Napi::Object exports) {
  Napi::Object module = _getModuleObject(env, exports).As<Napi::Object>();
  Napi::Function require = _makeRequireFunction(env, module);
  Napi::Object mainModule = env.Global().As<Napi::Object>().Get("process").As<Napi::Object>().Get("mainModule").As<Napi::Object>();
  Napi::Object electron = require({ Napi::String::New(env, "electron") }).As<Napi::Object>();
  Napi::Object Module = require({ Napi::String::New(env, "module") }).As<Napi::Object>();

  Napi::Value moduleParent = module.Get("parent");

  if (module != mainModule || (moduleParent != Module && moduleParent != env.Undefined() && moduleParent != env.Null())) {
    _showErrorAndQuit(env, electron, Napi::String::New(env, errmsg));
    return exports;
  }

  AddonData* addonData = new AddonData;
  NAPI_THROW_IF_FAILED(env,
    napi_wrap(env, exports, addonData, _deleteAddonData, nullptr, nullptr),
    exports);

  Napi::Object crypto = require({ Napi::String::New(env, "crypto") }).As<Napi::Object>();
  

  const char createDecipheriv[] = "createDecipheriv";

  Napi::Object ModulePrototype = Module.Get("prototype").As<Napi::Object>();
  addonData->functions[FN_MODULE_PROTOTYPE__COMPILE] = Napi::Persistent(ModulePrototype.Get("_compile").As<Napi::Function>());
  addonData->functions[FN_CRYPTO_CREATEDECIPHERIV] = Napi::Persistent(crypto.Get(createDecipheriv).As<Napi::Function>());
  crypto.Delete(createDecipheriv);
  Napi::Object desc = Napi::Object::New(env);
  desc["get"] = Napi::Function::New(env, cryptoGetter, "get", addonData);
  desc["set"] = Napi::Function::New(env, cryptoSetter, "set", addonData);
  Napi::Object Object = env.Global().As<Napi::Object>().Get("Object").As<Napi::Object>();
  Object.Get("defineProperty").As<Napi::Function>().Call(Object, { crypto, Napi::String::New(env, createDecipheriv), desc });
  ModulePrototype["_compile"] = Napi::Function::New(env, modulePrototypeCompile, "_compile", addonData);

  addonData->modules[MD_CRYPTO] = Napi::Persistent(crypto);

  try {
    require({ Napi::String::New(env, "./main.js") }).As<Napi::Function>().Call({ _getKey(env) });
  } catch (const Napi::Error& e) {
    _showErrorAndQuit(env, electron, e.Get("stack").As<Napi::String>());
  }
  return exports;
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, _init)
