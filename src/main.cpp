/*

const { app, dialog } = require('electron')
const crypto = require('crypto')
const Module = require('module')

function decrypt (body) {
  const iv = body.slice(0, 16)
  const hash = iv.toString('hex')
  const data = body.slice(16)
  const clearEncoding = 'utf8'
  const cipherEncoding = 'binary'
  const chunks = []
  const decipher = crypto.createDecipheriv('aes-256-cbc', '0123456789abcdef0123456789abcdef', iv)
  decipher.setAutoPadding(true)
  chunks.push(decipher.update(data, cipherEncoding, clearEncoding))
  chunks.push(decipher.final(clearEncoding))
  const code = chunks.join('')
  if (crypto.createHash('md5').update(code).digest('hex') !== hash) {
    throw new Error('This program has been changed by others.')
  }
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
  require('./main.js')
} catch (err) {
  dialog.showErrorBox('Error', err.stack)
  app.quit()
}

*/

#include <napi.h>
#include <unordered_map>

#define SECRET_KEY ("0123456789abcdef0123456789abcdef")

typedef struct AddonData {
  std::unordered_map<std::string, Napi::ObjectReference> modules;
  std::unordered_map<std::string, Napi::FunctionReference> functions;
} AddonData;

/* static void _log(Napi::Env env, Napi::Value value) {
  Napi::Object console = env.Global().As<Napi::Object>().Get("console").As<Napi::Object>();
  Napi::Function log = console.Get("log").As<Napi::Function>();
  log.Call(console, { value });
} */

static void _deleteAddonData(napi_env env, void* data, void* hint) {
  (void) env;
  (void) hint;
  // _log(env, Napi::String::New(env, "delete"));
  delete static_cast<AddonData*>(data);
}

static Napi::Value _decrypt(Napi::Env env, Napi::Object body, AddonData* addonData) {
  Napi::Object iv = body.Get("slice").As<Napi::Function>().Call(body, { Napi::Number::New(env, 0), Napi::Number::New(env, 16) }).As<Napi::Object>();
  Napi::String hash = iv.Get("toString").As<Napi::Function>().Call(iv, { Napi::String::New(env, "hex") }).As<Napi::String>();
  Napi::Object data = body.Get("slice").As<Napi::Function>().Call(body, { Napi::Number::New(env, 16) }).As<Napi::Object>();
  Napi::String clearEncoding = Napi::String::New(env, "utf8");
  Napi::String cipherEncoding = Napi::String::New(env, "binary");
  Napi::Array chunks = Napi::Array::New(env);
  Napi::Object crypto = addonData->modules["crypto"].Value();
  Napi::Object decipher = crypto.Get("createDecipheriv").As<Napi::Function>().Call(crypto, { Napi::String::New(env, "aes-256-cbc"), Napi::String::New(env, SECRET_KEY), iv }).As<Napi::Object>();
  decipher.Get("setAutoPadding").As<Napi::Function>().Call(decipher, { Napi::Boolean::New(env, true) });
  chunks.Get("push").As<Napi::Function>().Call(chunks, { decipher.Get("update").As<Napi::Function>().Call(decipher, { data, cipherEncoding, clearEncoding }) });
  chunks.Get("push").As<Napi::Function>().Call(chunks, { decipher.Get("final").As<Napi::Function>().Call(decipher, { clearEncoding }) });
  Napi::String code = chunks.Get("join").As<Napi::Function>().Call(chunks, { Napi::String::New(env, "") }).As<Napi::String>();

  Napi::Object md5 = crypto.Get("createHash").As<Napi::Function>().Call(crypto, { Napi::String::New(env, "md5") }).As<Napi::Object>();
  Napi::Object cipher = md5.Get("update").As<Napi::Function>().Call(md5, { code }).As<Napi::Object>();
  Napi::String _hash = cipher.Get("digest").As<Napi::Function>().Call(cipher, { Napi::String::New(env, "hex") }).As<Napi::String>();
  if (_hash.Utf8Value() != hash.Utf8Value()) {
    throw Napi::Error::New(env, "This program has been changed by others.");
  }
  return code;
}

static Napi::Value modulePrototypeCompile(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  AddonData* addonData = static_cast<AddonData*>(info.Data());
  Napi::Object content = info[0].As<Napi::Object>();
  Napi::Object filename = info[1].As<Napi::Object>();
  Napi::Function oldCompile = addonData->functions["Module.prototype._compile"].Value();

  if (-1 != filename.Get("indexOf").As<Napi::Function>().Call(filename, { Napi::String::New(env, "app.asar") }).As<Napi::Number>().Int32Value()) {
    Napi::Object Buffer = env.Global().Get("Buffer").As<Napi::Object>();
    Napi::Object body = Buffer.Get("from").As<Napi::Function>().Call(Buffer, { content, Napi::String::New(env, "base64") }).As<Napi::Object>();
    
    return oldCompile.Call(info.This(), { _decrypt(env, body, addonData), filename });
  }
  return oldCompile.Call(info.This(), { content, filename });
}

static Napi::Value _runScript(Napi::Env env, Napi::String script) {
  napi_value res;
  NAPI_THROW_IF_FAILED(env, napi_run_script(env, script, &res), env.Undefined());
  return Napi::Value(env, res);
}

static Napi::Value _runScript(Napi::Env env, const std::string& script) {
  return _runScript(env, Napi::String::New(env, script));
}

static Napi::Value _runScript(Napi::Env env, const char* script) {
  return _runScript(env, Napi::String::New(env, script));
}

static Napi::Function _makeRequireFunction(Napi::Env env, Napi::Object module) {
  std::string script = "(function makeRequireFunction(mod) {\n"
      "const Module = mod.constructor;\n"

      "function validateString (value, name) { if (typeof value !== 'string') throw new TypeError('The \"' + name + '\" argument must be of type string. Received type ' + typeof value); }\n"

      "const require = function require(path) {\n"
      "  return mod.require(path);\n"
      "};\n"

      "function resolve(request, options) {\n"
        "validateString(request, 'request');\n"
        "return Module._resolveFilename(request, mod, false, options);\n"
      "}\n"

      "require.resolve = resolve;\n"

      "function paths(request) {\n"
        "validateString(request, 'request');\n"
        "return Module._resolveLookupPaths(request, mod);\n"
      "}\n"

      "resolve.paths = paths;\n"

      "require.main = process.mainModule;\n"

      "require.extensions = Module._extensions;\n"

      "require.cache = Module._cache;\n"

      "return require;\n"
    "});";

  Napi::Function _makeRequire = _runScript(env, script).As<Napi::Function>();
  return _makeRequire({ module }).As<Napi::Function>();
}

static Napi::Value _getModuleObject(Napi::Env env, Napi::Object exports) {
  std::string findModuleScript = "(function (exports) {\n"
    "function findModule(start, target) {\n"
    "  if (start.exports === target) {\n"
    "    return start;\n"
    "  }\n"
    "  for (var i = 0; i < start.children.length; i++) {\n"
    "    var res = findModule(start.children[i], target);\n"
    "    if (res) {\n"
    "      return res;\n"
    "    }\n"
    "  }\n"
    "  return null;\n"
    "}\n"
    "return findModule(process.mainModule, exports);\n"
    "});";
  Napi::Function _findFunction = _runScript(env, findModuleScript).As<Napi::Function>();
  Napi::Value res = _findFunction({ exports });
  if (res.IsNull()) {
    Napi::Error::New(env, "Cannot find module object.").ThrowAsJavaScriptException();
  }
  return res;
}

static Napi::Object _init(Napi::Env env, Napi::Object exports) {
  Napi::Object module = _getModuleObject(env, exports).As<Napi::Object>();
  Napi::Function require = _makeRequireFunction(env, module);

  AddonData* addonData = new AddonData;
  NAPI_THROW_IF_FAILED(env,
    napi_wrap(env, exports, addonData, _deleteAddonData, nullptr, nullptr),
    exports);

  Napi::Object electron = require({ Napi::String::New(env, "electron") }).As<Napi::Object>();
  addonData->modules["crypto"] = Napi::Persistent(require({ Napi::String::New(env, "crypto") }).As<Napi::Object>());

  Napi::Object Module = require({ Napi::String::New(env, "module") }).As<Napi::Object>();

  Napi::Object ModulePrototype = Module.Get("prototype").As<Napi::Object>();
  addonData->functions["Module.prototype._compile"] = Napi::Persistent(ModulePrototype.Get("_compile").As<Napi::Function>());
  ModulePrototype["_compile"] = Napi::Function::New(env, modulePrototypeCompile, "_compile", addonData);

  try {
    require({ Napi::String::New(env, "./main.js") });
  } catch (const Napi::Error& e) {
    Napi::Object dialog = electron.Get("dialog").As<Napi::Object>();
    dialog.Get("showErrorBox").As<Napi::Function>().Call(dialog, { Napi::String::New(env, "Error"), e.Get("stack").As<Napi::String>() });

    Napi::Object app = electron.Get("app").As<Napi::Object>();
    Napi::Function quit = app.Get("quit").As<Napi::Function>();
    quit.Call(app, {});
  }
  return exports;
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, _init)
