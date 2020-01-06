/*

const dialog = require('electron').dialog
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
    dialog.showErrorBox('error', 'This program has been changed by others.')
    process.exit(1)
  }
  return chunks.join('')
}

const oldCompile = Module.prototype._compile

Module.prototype._compile = function (content, filename) {
  if (filename.indexOf('app.asar') !== -1) {
    return oldCompile.call(this, decrypt(Buffer.from(content, 'base64')), filename)
  }
  return oldCompile.call(this, content, filename)
}

*/

#include <napi.h>

#define SECRET_KEY ("0123456789abcdef0123456789abcdef")

static Napi::FunctionReference oldCompile;
static Napi::ObjectReference crypto;
static Napi::ObjectReference dialog;

/* static void _log(Napi::Env env, Napi::Value value) {
  Napi::Object console = env.Global().As<Napi::Object>().Get("console").As<Napi::Object>();
  Napi::Function log = console.Get("log").As<Napi::Function>();
  log.Call(console, { value });
} */

static Napi::Value _decrypt(Napi::Env env, Napi::Object body) {
  Napi::Object iv = body.Get("slice").As<Napi::Function>().Call(body, { Napi::Number::New(env, 0), Napi::Number::New(env, 16) }).As<Napi::Object>();
  Napi::String hash = iv.Get("toString").As<Napi::Function>().Call(iv, { Napi::String::New(env, "hex") }).As<Napi::String>();
  Napi::Object data = body.Get("slice").As<Napi::Function>().Call(body, { Napi::Number::New(env, 16) }).As<Napi::Object>();
  Napi::String clearEncoding = Napi::String::New(env, "utf8");
  Napi::String cipherEncoding = Napi::String::New(env, "binary");
  Napi::Array chunks = Napi::Array::New(env);
  Napi::Object decipher = crypto.Get("createDecipheriv").As<Napi::Function>().Call(crypto.Value(), { Napi::String::New(env, "aes-256-cbc"), Napi::String::New(env, SECRET_KEY), iv }).As<Napi::Object>();
  decipher.Get("setAutoPadding").As<Napi::Function>().Call(decipher, { Napi::Boolean::New(env, true) });
  chunks.Get("push").As<Napi::Function>().Call(chunks, { decipher.Get("update").As<Napi::Function>().Call(decipher, { data, cipherEncoding, clearEncoding }) });
  chunks.Get("push").As<Napi::Function>().Call(chunks, { decipher.Get("final").As<Napi::Function>().Call(decipher, { clearEncoding }) });
  Napi::String code = chunks.Get("join").As<Napi::Function>().Call(chunks, { Napi::String::New(env, "") }).As<Napi::String>();

  Napi::Object md5 = crypto.Get("createHash").As<Napi::Function>().Call(crypto.Value(), { Napi::String::New(env, "md5") }).As<Napi::Object>();
  Napi::Object cipher = md5.Get("update").As<Napi::Function>().Call(md5, { code }).As<Napi::Object>();
  Napi::String _hash = cipher.Get("digest").As<Napi::Function>().Call(cipher, { Napi::String::New(env, "hex") }).As<Napi::String>();
  if (_hash.Utf8Value() != hash.Utf8Value()) {
    dialog.Get("showErrorBox").As<Napi::Function>().Call(dialog.Value(), { Napi::String::New(env, "Error"), Napi::String::New(env, "This program has been changed by others.") });
    Napi::Object process = env.Global().Get("process").As<Napi::Object>();
    process.Get("exit").As<Napi::Function>().Call(process, { Napi::Number::New(env, 1) });
    return env.Undefined();
  }
  return code;
}

static Napi::Value ModulePrototypeOverrideCompile(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  Napi::Object content = info[0].As<Napi::Object>();
  Napi::Object filename = info[1].As<Napi::Object>();

  if (-1 != filename.Get("indexOf").As<Napi::Function>().Call(filename, { Napi::String::New(env, "app.asar") }).As<Napi::Number>().Int32Value()) {
    Napi::Object Buffer = env.Global().Get("Buffer").As<Napi::Object>();
    Napi::Object body = Buffer.Get("from").As<Napi::Function>().Call(Buffer, { content, Napi::String::New(env, "base64") }).As<Napi::Object>();
    
    return oldCompile.Call(info.This(), { _decrypt(env, body), filename });
  }
  return oldCompile.Call(info.This(), { content, filename });
}

static Napi::Value run(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (info.Length() != 6) {
    Napi::Error::New(env, "main.run(): args.length !== 6").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  if (!info[0].IsObject()) {
    Napi::Error::New(env, "main.run(): typeof this !== 'object'").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  if (!info[1].IsObject()) {
    Napi::Error::New(env, "main.run(): typeof module !== 'object'").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  if (!info[2].IsObject()) {
    Napi::Error::New(env, "main.run(): typeof exports !== 'object'").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  if (!info[3].IsFunction()) {
    Napi::Error::New(env, "main.run(): typeof require !== 'function'").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  if (!info[4].IsString()) {
    Napi::Error::New(env, "main.run(): typeof __dirname !== 'string'").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  if (!info[5].IsString()) {
    Napi::Error::New(env, "main.run(): typeof __filename !== 'string'").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  Napi::Function require = info[3].As<Napi::Function>();

  dialog = Napi::Weak(require({ Napi::String::New(env, "electron") }).As<Napi::Object>().Get("dialog").As<Napi::Object>());
  crypto = Napi::Weak(require({ Napi::String::New(env, "crypto") }).As<Napi::Object>());
  Napi::Object Module = require({ Napi::String::New(env, "module") }).As<Napi::Object>();

  Napi::Object ModulePrototype = Module.Get("prototype").As<Napi::Object>();
  oldCompile = Napi::Weak(ModulePrototype.Get("_compile").As<Napi::Function>());
  ModulePrototype["_compile"] = Napi::Function::New(env, ModulePrototypeOverrideCompile, "_compile");

  require({ Napi::String::New(env, "./main.js") });
  return env.Undefined();
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  return Napi::Function::New(env, run);
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)
