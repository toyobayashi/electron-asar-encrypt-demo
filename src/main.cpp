/*
for (let i = 0; i < process.argv.length; i++) {
  if (process.argv[i].startsWith('--inspect') ||
      process.argv[i].startsWith('--remote-debugging-port')) {
    throw new Error('Not allow debugging this program.')
  }
}

const { app, dialog } = require('electron')

const moduleParent = module.parent;
if (module !== process.mainModule || (moduleParent !== Module && moduleParent !== undefined && moduleParent !== null)) {
  dialog.showErrorBox('Error', 'This program has been changed by others.')
  app.quit()
}

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
#include <unordered_map>
#include "napi.h"
#include "script.h"

#include "aes/aes.hpp"

#define KEY_LENGTH 32

#define FN_MODULE_PROTOTYPE__COMPILE 0

namespace {

struct AddonData {
  // std::unordered_map<int, Napi::ObjectReference> modules;
  std::unordered_map<int, Napi::FunctionReference> functions;
};

const char errmsg[] = "This program has been changed by others.";

void ConsoleLog(const Napi::Env& env, Napi::Value value) {
  Napi::Object console = env.Global().As<Napi::Object>()
    .Get("console").As<Napi::Object>();
  Napi::Function log = console.Get("log").As<Napi::Function>();
  log.Call(console, { value });
}

void ConsoleError(const Napi::Env& env, Napi::Value value) {
  Napi::Object console = env.Global().As<Napi::Object>()
    .Get("console").As<Napi::Object>();
  Napi::Function error = console.Get("error").As<Napi::Function>();
  error.Call(console, { value });
}

std::vector<uint8_t> GetKeyVector() {
  const uint8_t key[KEY_LENGTH] = {
#include "key.txt"
  };

  return std::vector<uint8_t>(key, key + KEY_LENGTH);
}

Napi::Array GetKey(const Napi::Env& env) {
  std::vector<uint8_t> key = GetKeyVector();
  Napi::Array arrkey = Napi::Array::New(env, key.size());
  for (uint32_t i = 0; i < key.size(); i++) {
    arrkey.Set(i, key[i]);
  }
  return arrkey;
}

int Pkcs7cut(uint8_t *p, int plen) {
  uint8_t last = p[plen - 1];
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

std::string Aesdec(const std::vector<uint8_t>& data,
                   const std::vector<uint8_t>& key,
                   const std::vector<uint8_t>& iv) {
  size_t l = data.size();
  uint8_t* encrypt = new uint8_t[l];
  memcpy(encrypt, data.data(), l);

  struct AES_ctx ctx;
  AES_init_ctx_iv(&ctx, key.data(), iv.data());
  AES_CBC_decrypt_buffer(&ctx, encrypt, l);

  uint8_t* out = new uint8_t[l + 1];
  memcpy(out, encrypt, l);
  out[l] = 0;

  int realLength = Pkcs7cut(out, l);
  out[realLength] = 0;

  delete[] encrypt;
  std::string res = reinterpret_cast<char*>(out);
  delete[] out;
  return res;
}

std::vector<uint8_t> BufferToVector(const Napi::Buffer<uint8_t>& buf) {
  uint8_t* data = buf.Data();
  return std::vector<uint8_t>(data, data + buf.ByteLength());
}

Napi::String Base64toCode(const Napi::Env& env,
                          const Napi::String& base64) {
  Napi::Object buffer_constructor = env.Global().Get("Buffer")
    .As<Napi::Object>();
  Napi::Buffer<uint8_t> body = buffer_constructor.Get("from")
    .As<Napi::Function>()
    .Call(buffer_constructor, { base64, Napi::String::New(env, "base64") })
    .As<Napi::Buffer<uint8_t>>();

  Napi::Buffer<uint8_t> iv = body.Get("slice").As<Napi::Function>()
    .Call(body, { Napi::Number::New(env, 0), Napi::Number::New(env, 16) })
    .As<Napi::Buffer<uint8_t>>();
  Napi::Buffer<uint8_t> data = body.Get("slice").As<Napi::Function>()
    .Call(body, { Napi::Number::New(env, 16) })
    .As<Napi::Buffer<uint8_t>>();

  std::string plain_content = Aesdec(BufferToVector(data),
    GetKeyVector(), BufferToVector(iv));

  return Napi::String::New(env, plain_content);
}

Napi::Value ModulePrototypeCompile(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  AddonData* addon_data = static_cast<AddonData*>(info.Data());
  Napi::Object content = info[0].As<Napi::Object>();
  Napi::Object filename = info[1].As<Napi::Object>();
  Napi::Function old_compile =
    addon_data->functions[FN_MODULE_PROTOTYPE__COMPILE].Value();

  if (-1 != filename.Get("indexOf").As<Napi::Function>()
              .Call(filename, { Napi::String::New(env, "app.asar") })
              .As<Napi::Number>().Int32Value()) {
    return old_compile.Call(info.This(),
      { Base64toCode(env, content.As<Napi::String>()), filename });
  }
  return old_compile.Call(info.This(), { content, filename });
}

Napi::Function MakeRequireFunction(Napi::Env* env,
                                   const Napi::Object& mod) {
  Napi::Function make_require = env->RunScript(scriptRequire)
    .As<Napi::Function>();
  return make_require({ mod }).As<Napi::Function>();
}

Napi::Value GetModuleObject(Napi::Env* env,
                            const Napi::Object& main_module,
                            const Napi::Object& this_exports) {
  Napi::Function find_function = env->RunScript(scriptFind)
    .As<Napi::Function>();
  Napi::Value res = find_function({ main_module, this_exports });
  if (res.IsNull()) {
    Napi::Error::New(*env, "Cannot find module object.")
      .ThrowAsJavaScriptException();
  }
  return res;
}

void ShowErrorAndQuit(const Napi::Env& env,
                      const Napi::Object& electron,
                      const Napi::String& message) {
  Napi::Value ELECTRON_RUN_AS_NODE = env.Global().As<Napi::Object>()
    .Get("process").As<Napi::Object>()
    .Get("env").As<Napi::Object>()
    .Get("ELECTRON_RUN_AS_NODE");

  if (!ELECTRON_RUN_AS_NODE.IsUndefined() &&
      ELECTRON_RUN_AS_NODE != Napi::Number::New(env, 0) &&
      ELECTRON_RUN_AS_NODE != Napi::String::New(env, "")) {
    ConsoleError(env, message);
    exit(1);
  } else {
    Napi::Object dialog = electron.Get("dialog").As<Napi::Object>();
    dialog.Get("showErrorBox").As<Napi::Function>()
      .Call(dialog, { Napi::String::New(env, "Error"), message });

    Napi::Object app = electron.Get("app").As<Napi::Object>();
    Napi::Function quit = app.Get("quit").As<Napi::Function>();
    quit.Call(app, {});
  }
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
#ifdef _TARGET_ELECTRON_RENDERER_
  Napi::Object main_module = env.Global().Get("module").As<Napi::Object>();
#else
  Napi::Object process = env.Global().Get("process").As<Napi::Object>();
  Napi::Array argv = process.Get("argv").As<Napi::Array>();
  for (uint32_t i = 0; i < argv.Length(); ++i) {
    std::string arg = argv.Get(i).As<Napi::String>().Utf8Value();
    if (arg.find("--inspect") == 0 ||
        arg.find("--remote-debugging-port") == 0) {
      Napi::Error::New(env, "Not allow debugging this program.")
        .ThrowAsJavaScriptException();
      return exports;
    }
  }
  Napi::Object main_module = process.Get("mainModule").As<Napi::Object>();
#endif

  Napi::Object this_module = GetModuleObject(&env, main_module, exports)
    .As<Napi::Object>();
  Napi::Function require = MakeRequireFunction(&env, this_module);

  Napi::Object electron = require({ Napi::String::New(env, "electron") })
    .As<Napi::Object>();
  Napi::Object module_constructor = require({
    Napi::String::New(env, "module") }).As<Napi::Object>();

  Napi::Value module_parent = this_module.Get("parent");

#ifdef _TARGET_ELECTRON_RENDERER_
  if (module_parent != main_module) {
    Napi::Object ipcRenderer = electron.Get("ipcRenderer").As<Napi::Object>();
    Napi::Function sendSync = ipcRenderer.Get("sendSync").As<Napi::Function>();
    sendSync.Call(ipcRenderer,
      { Napi::String::New(env, "__SHOW_ERROR_AND_QUIT__") });
    return exports;
  }
#else
  if (this_module != main_module || (
    module_parent != module_constructor &&
    module_parent != env.Undefined() &&
    module_parent != env.Null())) {
    ShowErrorAndQuit(env, electron, Napi::String::New(env, errmsg));
    return exports;
  }
#endif

  AddonData* addon_data = env.GetInstanceData<AddonData>();

  if (addon_data == nullptr) {
    addon_data = new AddonData();
    env.SetInstanceData(addon_data);
  }

  Napi::Object module_prototype = module_constructor.Get("prototype")
    .As<Napi::Object>();
  addon_data->functions[FN_MODULE_PROTOTYPE__COMPILE] =
    Napi::Persistent(module_prototype.Get("_compile").As<Napi::Function>());

  module_prototype.DefineProperty(
    Napi::PropertyDescriptor::Function(env,
      Napi::Object::New(env),
      "_compile",
      ModulePrototypeCompile,
      napi_enumerable,
      addon_data));

#ifdef _TARGET_ELECTRON_RENDERER_
  return exports;
#else

  Napi::Value ELECTRON_RUN_AS_NODE = env.Global().As<Napi::Object>()
    .Get("process").As<Napi::Object>()
    .Get("env").As<Napi::Object>()
    .Get("ELECTRON_RUN_AS_NODE");

  if (ELECTRON_RUN_AS_NODE.IsUndefined() ||
      ELECTRON_RUN_AS_NODE == Napi::Number::New(env, 0) ||
      ELECTRON_RUN_AS_NODE == Napi::String::New(env, "")) {
    Napi::Object ipcMain = electron.Get("ipcMain").As<Napi::Object>();
    Napi::Function once = ipcMain.Get("once").As<Napi::Function>();

    once.Call(ipcMain, {
      Napi::String::New(env, "__SHOW_ERROR_AND_QUIT__"),
      Napi::Function::New(env,
        [](const Napi::CallbackInfo& info) -> Napi::Value {
          Napi::Env env = info.Env();
          Napi::Object event = info[0].As<Napi::Object>();
          Napi::Object mm = env.Global().Get("process").As<Napi::Object>()
            .Get("mainModule").As<Napi::Object>();
          Napi::Function req = mm.Get("require").As<Napi::Function>();
          ShowErrorAndQuit(env,
            req.Call(mm, { Napi::String::New(env, "electron") })
              .As<Napi::Object>(),
            Napi::String::New(env, errmsg));
          event.Set("returnValue", env.Null());
          return env.Undefined();
        })
    });
  }

  try {
    require({ Napi::String::New(env, "./main.js") })
      .As<Napi::Function>().Call({ GetKey(env) });
  } catch (const Napi::Error& e) {
    ShowErrorAndQuit(env, electron, e.Get("stack").As<Napi::String>());
  }
  return exports;
#endif
}

}  // namespace

NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)
