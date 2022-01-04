# 给 Electron 应用加密源码

## 为什么会有这个仓库？

众所周知，[Electron](https://electronjs.org) 官方没有提供保护源码的方法。打包一个 Electron 应用，说白了就是[把源码拷到固定的一个地方](http://electronjs.org/docs/tutorial/application-distribution)，比如在 Windows / Linux 下是 `resources/app` 这个目录。运行 Electron 应用时，Electron 就把这个目录当作一个 Node.js 项目去跑里面的 JS 代码。虽然 Electron 认识 ASAR 格式的代码包，即可以把所有的源码打包成一个 `app.asar` 文件放到 `resources` 目录，Electron 把 `app.asar` 当成一个文件夹，跑里面的代码，但是 ASAR 包中的文件是没有加密的，仅仅是把所有的文件拼接成了一个文件再加上了文件头信息，使用官方提供的 `asar` 库很容易把所有的源码从 ASAR 包提取出来，所以起不到加密的效果，只是对于初心者来说想接触到源码多了一点小门槛，稍微懂行一点的完全无压力。

所以我就在思考如何对 ASAR 包进行加密，防止商业源码被一些有心人士轻易篡改或注入一些恶意的代码后再分发。这里提供了一种不需要重新编译 Electron 即可完成加密的思路。

## 跑起来

``` bash
git clone https://github.com/toyobayashi/electron-asar-encrypt-demo.git
cd ./electron-asar-encrypt-demo
npm install # 复制 electron release 到 test 目录
npm start # 编译然后启动应用
npm test # 编译然后跑测试
```

## 加密

以 AES-256-CBC 为例，先生成密钥保存在本地的文件中，方便 JS 打包脚本导入和 C++ include 内联。

``` js
// 这个脚本不会被打包进客户端，本地开发用
const fs = require('fs')
const path = require('path')
const crypto = require('crypto')

fs.writeFileSync(path.join(__dirname, 'src/key.txt'), Array.prototype.map.call(crypto.randomBytes(32), (v => ('0x' + ('0' + v.toString(16)).slice(-2)))))
```

这样就会在 `src` 生成一个 `key.txt` 文件，里面的内容是这样的：

```
0x87,0xdb,0x34,0xc6,0x73,0xab,0xae,0xad,0x4b,0xbe,0x38,0x4b,0xf5,0xd4,0xb5,0x43,0xfe,0x65,0x1c,0xf5,0x35,0xbb,0x4a,0x78,0x0a,0x78,0x61,0x65,0x99,0x2a,0xf1,0xbb
```

打包时做加密，利用 `asar` 库的 `asar.createPackageWithOptions()` 这个 API：

``` ts
/// <reference types="node" />

declare namespace asar {
  // ...
  export function createPackageWithOptions(
    src: string,
    dest: string,
    options: {
      // ...
      transform?: (filePath: string) => NodeJS.ReadWriteStream | void;
    }
  ): Promise<void>
}

export = asar;
```

在第三个参数中传入 `transform` 选项，它是一个函数，返回一个 `ReadWriteStream` 可读写流去处理文件，返回 `undefined` 则不对文件做处理。这一步对所有的 JS 文件加密后打进 ASAR 包中。

``` js
// 这个脚本不会被打包进客户端，本地开发用

const crypto = require('crypto')
const path = require('path')
const fs = require('fs')
const asar = require('asar')

// 读取密钥，弄成一个 Buffer
const key = Buffer.from(fs.readFileSync(path.join(__dirname, 'src/key.txt'), 'utf8').trim().split(',').map(v => Number(v.trim())))

asar.createPackageWithOptions(
  path.join(__dirname, './app'),
  path.join(__dirname, './test/resources/app.asar'),
  {
    unpack: '*.node', // C++ 模块不打包
    transform (filename) {
      if (path.extname(filename) === '.js') {
        // 生成随机的 16 字节初始化向量 IV
        const iv = crypto.randomBytes(16)

        // 是否已把 IV 拼在了加密后数据的
        let append = false

        const cipher = crypto.createCipheriv(
          'aes-256-cbc',
          key,
          iv
        )
        cipher.setAutoPadding(true)
        cipher.setEncoding('base64')

        // 重写 Readable.prototype.push 把 IV 拼在加密后数据的最前面
        const _p = cipher.push
        cipher.push = function (chunk, enc) {
          if (!append && chunk != null) {
            append = true
            return _p.call(this, Buffer.concat([iv, chunk]), enc)
          } else {
            return _p.call(this, chunk, enc)
          }
        }
        return cipher
      }
    }
  }
)
```

## 主进程解密

解密在客户端运行时做，因为 V8 引擎没法运行加密后的 JS，所以必须先解密后再丢给 V8 跑。这里就有讲究了，客户端代码是可以被任何人蹂躏的，所以密钥不能明着写，也不能放配置文件，所以只能下沉到 C++ 里。用 C++ 写一个原生模块实现解密，而且这个模块不能导出解密方法，否则没有意义。另外在 C++ 源码中密钥也不能写死成字符串，因为字符串在编译后的二进制文件中是可以直接找到的。

什么？不导出不是没法用吗？很简单，Hack 掉 Node.js 的 API，保证外部拿不到就 OK，然后直接把这个原生模块当作入口模块，在原生模块里面再 require 一下真正的入口 JS。以下是等价的 JS 逻辑：

``` js
// 用 C++ 写以下逻辑，可以使密钥被编译进动态库
// 只有反编译动态库才有可能分析出来

// 禁止调试
for (let i = 0; i < process.argv.length; i++) {
  if (process.argv[i].startsWith('--inspect') ||
      process.argv[i].startsWith('--remote-debugging-port')) {
    throw new Error('Not allow debugging this program.')
  }
}

const { app, dialog } = require('electron')

const moduleParent = module.parent;
if (module !== process.mainModule || (moduleParent !== Module && moduleParent !== undefined && moduleParent !== null)) {
  // 如果该原生模块不是入口，就报错退出
  dialog.showErrorBox('Error', 'This program has been changed by others.')
  app.quit()
}

const Module = require('module')

function getKey () {
  // 在这里内联由 JS 脚本生成的密钥
  // const unsigned char key[32] = {
  //   #include "key.txt"
  // };
  return KEY
}

function decrypt (body) { // body 是 Buffer
  const iv = body.slice(0, 16) // 前 16 字节是 IV
  const data = body.slice(16) // 16 字节以后是加密后的代码

  // 最好使用原生库来做解密，Node API 存在被拦截的风险

  // const clearEncoding = 'utf8' // 输出是字符串
  // const cipherEncoding = 'binary' // 输入是二进制
  // const chunks = [] // 保存分段的字符串
  // const decipher = require('crypto').createDecipheriv(
  //   'aes-256-cbc',
  //   getKey(),
  //   iv
  // )
  // decipher.setAutoPadding(true)
  // chunks.push(decipher.update(data, cipherEncoding, clearEncoding))
  // chunks.push(decipher.final(clearEncoding))
  // const code = chunks.join('')
  // return code

  // [native code]
}

const oldCompile = Module.prototype._compile
// 重写 Module.prototype._compile
// 原因就不多写了，看下 Node 的源码就知道
Object.defineProperty(Module.prototype, '_compile', {
  enumerable: true,
  value: function (content, filename) {
    if (filename.indexOf('app.asar') !== -1) {
      // 如果这个 JS 是在 app.asar 里面，就先解密
      return oldCompile.call(this, decrypt(Buffer.from(content, 'base64')), filename)
    }
    return oldCompile.call(this, content, filename)
  }
})

try {
  // 主进程创建窗口在这里面，如果需要的话把密钥传给 JS，最好不要传
  require('./main.js')(getKey()) 
} catch (err) {
  // 防止 Electron 报错后不退出
  dialog.showErrorBox('Error', err.stack)
  app.quit()
}
```

要使用 C++ 写出上面的代码，有一个问题，如何在 C++ 里拿到 JS 的 `require` 函数呢？

看下 Node 源码就知道，调用 `require` 就是相当于调用 `Module.prototype.require`，所以只要能拿到 `module` 对象，也就能够拿到 `require` 函数。不幸的是，NAPI 没有在模块初始化的回调中暴露 `module` 对象，有人提了 PR 但是官方似乎考虑到某些原因（向 ES Module 标准看齐）并不想暴露 `module`，只暴露了 `exports` 对象，不像 Node CommonJS 模块中 JS 代码被一层函数包裹：

``` js
function (exports, require, module, __filename, __dirname) {
  // 自己写的代码在这里
}
```

仔细翻阅 Node.js 文档，可以在 process 章节里看到有 `global.process.mainModule` 这么个东西，也就是说入口模块是可以从全局拿到的，只要遍历模块的 `children` 数组往下找，对比 `module.exports` 等不等于 `exports`，就可以找到当前原生模块的 `module` 对象。

先封装一下运行脚本的方法。

``` cpp
#include <string>
#include "napi.h"

// 先封装一下脚本运行方法
Napi::Value RunScript(Napi::Env& env, const Napi::String& script) {
  napi_value res;
  NAPI_THROW_IF_FAILED(env, napi_run_script(env, script, &res), env.Undefined());
  return Napi::Value(env, res); // env.RunScript(script);
}

Napi::Value RunScript(Napi::Env& env, const std::string& script) {
  return RunScript(env, Napi::String::New(env, script)); // env.RunScript(script);
}

Napi::Value RunScript(Napi::Env& env, const char* script) {
  return RunScript(env, Napi::String::New(env, script)); // env.RunScript(script);
}
```

`node-addon-api` v3 以上可以直接使用：

``` cpp
Napi::Value Napi::Env::RunScript(const char* utf8script);
Napi::Value Napi::Env::RunScript(const std::string& utf8script);
Napi::Value Napi::Env::RunScript(Napi::String script);
```

然后就可以愉快地 JS in C++ 了。

``` cpp
Napi::Value GetModuleObject(Napi::Env& env, const Napi::Object& main_module, const Napi::Object& exports) {
  std::string script = "(function (mainModule, exports) {\n"
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
    "return findModule(mainModule, exports);\n"
    "});";
  Napi::Function find_function = RunScript(env, script).As<Napi::Function>();
  Napi::Value res = find_function({ main_module, exports });
  if (res.IsNull()) {
    Napi::Error::New(env, "Cannot find module object.").ThrowAsJavaScriptException();
  }
  return res;
}
Napi::Function MakeRequireFunction(Napi::Env& env, const Napi::Object& mod) {
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

  Napi::Function make_require = RunScript(env, script).As<Napi::Function>();
  return make_require({ mod }).As<Napi::Function>();
}
```

``` cpp
#include <unordered_map>

struct AddonData {
  // 存 Node 模块引用
  // std::unordered_map<std::string, Napi::ObjectReference> modules;
  // 存函数引用
  std::unordered_map<std::string, Napi::FunctionReference> functions;
};

Napi::Value ModulePrototypeCompile(const Napi::CallbackInfo& info) {
  AddonData* addon_data = static_cast<AddonData*>(info.Data());
  Napi::Function old_compile = addon_data->functions["Module.prototype._compile"].Value();
  // 这里推荐使用 C/C++ 的库来做解密
  // ...
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
#ifdef _TARGET_ELECTRON_RENDERER_
  // const mainModule = window.module
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
  // const mainModule = process.mainModule
  Napi::Object main_module = process.Get("mainModule").As<Napi::Object>();
#endif

  Napi::Object this_module = GetModuleObject(&env, main_module, exports).As<Napi::Object>();
  Napi::Function require = MakeRequireFunction(env, this_module);
  // const mainModule = process.mainModule
  Napi::Object main_module = env.Global().As<Napi::Object>().Get("process").As<Napi::Object>().Get("mainModule").As<Napi::Object>();
  // const electron = require('electron')
  Napi::Object electron = require({ Napi::String::New(env, "electron") }).As<Napi::Object>();
  // require('module')
  Napi::Object module_constructor = require({ Napi::String::New(env, "module") }).As<Napi::Object>();
  // module.parent
  Napi::Value module_parent = this_module.Get("parent");

  if (this_module != main_module ||
      (module_parent != module_constructor && module_parent != env.Undefined() && module_parent != env.Null())) {
    // 入口模块不是当前的原生模块，可能会被拦截 API 导致泄露密钥
    // 弹窗警告后退出
  }

  AddonData* addon_data = env.GetInstanceData<AddonData>();

  if (addon_data == nullptr) {
    addon_data = new AddonData();
    env.SetInstanceData(addon_data);
  }

  // require('crypto')
  // addon_data->modules["crypto"] = Napi::Persistent(require({ Napi::String::New(env, "crypto") }).As<Napi::Object>());

  Napi::Object module_prototype = module_constructor.Get("prototype").As<Napi::Object>();
  addon_data->functions["Module.prototype._compile"] = Napi::Persistent(module_prototype.Get("_compile").As<Napi::Function>());
  module_prototype["_compile"] = Napi::Function::New(env, ModulePrototypeCompile, "_compile", addon_data);

  try {
    require({ Napi::String::New(env, "./main.js") }).Call({ getKey() });
  } catch (const Napi::Error& e) {
    // 弹窗后退出
    // ...
  }
  return exports;
}

// 不要分号，NODE_API_MODULE 是个宏
NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)
```

看到这里可能会问了搞了大半天为什么还要用 C++ 来写 JS 啊，不是明明可以 `RunScript()` 吗？ 前面也已经提到过，直接 runScript 需要把 JS 写死成字符串，在编译后的二进制文件中是原样存在的，密钥会被泄露，用 C++ 来写这些逻辑可以增加反向的难度。

总结下就是这样的：

1. `main.node` （已编译） 里面 require `main.js` （已加密）
2. `main.js` （已加密）里面再 require 其它加密的 JS，创建窗口等等

特别注意，入口必须是 main.node，如果不是，则很有可能被攻击者在加载 main.node 之前的 JS 中 hack 掉 Node API 导致密钥泄露。比如这样的入口文件：

``` js
const crypto = require('crypto')

const old = crypto.createDecipheriv
crypto.createDecipheriv = function (...args) {
  console.log(...args) // 密钥被输出
  return old.call(crypto, ...args)
}

const Module = require('module')

const oldCompile = Module.prototype._compile
      
Module.prototype._compile = function (content, filename) {
  console.log(content) // JS 源码被输出
  return oldCompile.call(this, content, filename)
}

process.argv.length = 1

require('./main.node')
// 或 Module._load('./main.node', module, true)
```

## 渲染进程解密

和主进程的逻辑类似，可以利用预定义宏在 C++ 中区分开主进程和渲染进程。为渲染进程再编译出一个 `renderer.node`。渲染进程加载的原生模块必须是 `上下文感知模块`，用 NAPI 写的模块已经是上下文感知的，所以没有问题，如果用 V8 的 API 去写就是不行的。

这里有个限制，不能在 HTML 中直接引用 `<script>` 标签加载 JS，因为 HTML 中的 `<script>` 不走 `Module.prototype._compile`，所以只能在主进程中调用 `browserWindow.webContents.executeJavaScript()` 来为每个窗口最先加载原生模块，然后再 require 其它可能需要解密的 JS 文件。

## 局限性

* 必须开启 `nodeIntegration` 选项。也不能使用 `preload` 预加载脚本，因为这样原生模块会找不到自身 `module` 实例，也就无法使用 `require`
* 只能加密 JS，不能加密其它类型的文件，如 JSON、图片资源等
* 所有不走 `Module.prototype._compile` 的 JS 加载方法都不能加载加密后的 JS，例如依赖 HTML `<script>` 标签的脚本加载方式失效，Webpack 动态导入 `import()` 失效
* 如果 JS 文件很多，解密造成的性能影响较大，下面会说如何减少需要加密的 JS
* 不能使用纯 JS 实现，必须要 C++ 编译关键的密钥和解密方法
* 不能做成收费应用
* 不能算绝对安全，反编译原生模块仍然有密钥泄露和加密方法被得知的风险，只是相对于单纯的 ASAR 打包来说稍微提高了一点破解的门槛，源码不是那么容易被接触。如果有人真想蹂躏你的代码，这种方法防御力可能还远远不够

最有效的方法是改 Electron 源码，重新编译 Electron。但是，动源码技术门槛高，重新编译 Electron 需要科学那什么，而且编译超慢。

## 减少需要加密的 JS

`node_modules` 里面的 JS 很多，且不需要加密，所以可以单独抽出来一个 `node_modules.asar`，这里面的 JS 不加密。但是这样会给逆向带来更多机会，别人可以在这些 NPM 包中注入他们想要运行的 JS 代码，有风险。

如何让 `require` 找到 `node_modules.asar` 内部的库呢？答案同样是 Hack 掉 Node 的 API。

``` js
const path = require('path')
const Module = require('module')

const originalResolveLookupPaths = Module._resolveLookupPaths

Module._resolveLookupPaths = originalResolveLookupPaths.length === 2 ? function (request, parent) {
  // Node v12+
  const result = originalResolveLookupPaths.call(this, request, parent)

  if (!result) return result

  for (let i = 0; i < result.length; i++) {
    if (path.basename(result[i]) === 'node_modules') {
      result.splice(i + 1, 0, result[i] + '.asar')
      i++
    }
  }

  return result
} : function (request, parent, newReturn) {
  // Node v10-
  const result = originalResolveLookupPaths.call(this, request, parent, newReturn)

  const paths = newReturn ? result : result[1]
  for (let i = 0; i < paths.length; i++) {
    if (path.basename(paths[i]) === 'node_modules') {
      paths.splice(i + 1, 0, paths[i] + '.asar')
      i++
    }
  }

  return result
}
```

这样把 `node_modules` 打成 `node_modules.asar` 放到 `resources` 文件夹下和 `app.asar` 同级也 OK 了。

注意记得 unpack `*.node` 原生模块。

## 总结

打包时做加密，运行时做解密，解密逻辑放 C++ 里，而且必须是最先加载。

最后关键，不要在预加载的代码中 `console.log`，不要忘了在生产环境关掉 `devTools` 和打开 `nodeIntegration`：

``` js
new BrowserWindow({
  // ...
  webPreferences: {
    nodeIntegration: true, // 渲染进程要使用 require
    contextIsolation: false, // Electron 12 开始默认值为 true，要关掉
    devTools: false // 关掉开发者工具，因为开发者工具可以看到渲染进程的代码
  }
})
```
