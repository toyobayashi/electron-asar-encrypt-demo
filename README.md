# 给 Electron 应用加密源码

## 为什么会有这个仓库？

众所周知，[Electron](https://electronjs.org) 官方没有提供保护源码的方法。打包一个 Electron 应用，说白了就是[把源码拷到固定的一个地方](http://electronjs.org/docs/tutorial/application-distribution)，比如在 Windows / Linux 下是 `resources/app` 这个目录。运行 Electron 应用时，Electron 就把这个目录当作一个 Node.js 项目去跑里面的 JS 代码。虽然 Electron 认识 ASAR 格式的代码包，即可以把所有的源码打包成一个 `app.asar` 文件放到 `resources` 目录，Electron 把 `app.asar` 当成一个文件夹，跑里面的代码，但是 ASAR 包中的文件是没有加密的，仅仅是把所有的文件拼接成了一个文件再加上了文件头信息，使用官方提供的 `asar` 库很容易把所有的源码从 ASAR 包提取出来，所以起不到加密的效果，只是对于初心者来说想接触到源码多了一点小门槛，稍微懂行一点的完全无压力。

所以我就在思考如何对 ASAR 包进行加密，防止商业源码被一些有心人士轻易篡改或注入一些恶意的代码后再分发。这里提供了一种不需要重新编译 Electron 即可完成加密的思路。

## 加密

打包时做加密，利用 `asar` 库的 `asar.createPackageWithOptions()` 这个 API。（官方没有提供 `.d.ts` 有点烦，这里随便写一下

``` ts
declare namespace asar {
  // ...
  export function createPackageWithOptions(
    src: string,
    dest: string,
    options: {
      // ...
      transform? (filename: string): import('stream').Tansform | undefined;
    }
  )
}

export = asar;
```

在第三个参数中传入 `transform` 选项，它是一个函数，返回一个转换流处理文件，返回 `undefined` 则不对文件做处理。这一步对所有的 JS 文件加密后打进 ASAR 包中，除了入口模块，原因下面会说。

``` js
// 这个脚本不会被打包进客户端，本地开发用，不必担心密钥用明文写

const crypto = require('crypto')
const path = require('path')
const fs = require('fs')
const asar = require('asar')

asar.createPackageWithOptions(
  path.join(__dirname, './app'),
  path.join(__dirname, './test/resources/app.asar'),
  {
    unpack: '*.node', // C++ 模块不打包
    transform (filename) {
      if (path.extname(filename) === '.js' &&
          path.basename(filename) !== 'index.js') { // 入口模块不加密
        // 算 JS 文件的 MD5
        const hash = crypto.createHash('md5').update(
          fs.readFileSync(filename)
        ).digest('hex')

        // 把 MD5 转成 16 字节长的 Buffer 当作加密用的 IV
        const iv = Buffer.from(hash, 'hex')

        // 是否已把 IV 拼在了加密后数据的
        let append = false

        const cipher = crypto.createCipheriv(
          'aes-256-cbc',
          '0123456789abcdef0123456789abcdef',
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

解密在客户端运行时做，因为 V8 引擎没法运行加密后的 JS，所以必须先解密后再丢给 V8 跑。这里就有讲究了，客户端代码是可以被任何人蹂躏的，所以密钥不能明着写，也不能放配置文件，所以只能下沉到 C++ 里。用 C++ 写一个原生模块实现解密，而且这个模块不能导出解密方法，否则没有意义。

什么？不导出不是没法用吗？很简单，Hack 掉 Node.js 的 API，保证外部拿不到就 OK，然后直接在原生模块里面再 require 一下真正的入口 JS。以下是等价的 JS 逻辑：

``` js
// 用 C++ 写以下逻辑，可以使密钥被编译进动态库
// 只有反编译动态库才有可能分析出来
const dialog = require('electron').dialog
const crypto = require('crypto')
const Module = require('module')

function decrypt (body) { // body 是 Buffer
  const iv = body.slice(0, 16) // 前 16 字节是 IV
  const hash = iv.toString('hex') // 源码的 MD5
  const data = body.slice(16) // 16 字节以后是加密后的代码
  const clearEncoding = 'utf8' // 输出是字符串
  const cipherEncoding = 'binary' // 输入是二进制
  const chunks = [] // 保存分段的字符串
  const decipher = crypto.createDecipheriv(
    'aes-256-cbc',
    '0123456789abcdef0123456789abcdef',
    iv
  )
  decipher.setAutoPadding(true)
  chunks.push(decipher.update(data, cipherEncoding, clearEncoding))
  chunks.push(decipher.final(clearEncoding))
  const code = chunks.join('')
  if (crypto.createHash('md5').update(code).digest('hex') !== hash) {
    // 如果不一样，说明肯定有问题
    dialog.showErrorBox('error', 'This program has been changed by others.')
    process.exit(1)
  }
  // 注意：如果一样，不能说明源码没被改过，只能信他了
  return code
}

const oldCompile = Module.prototype._compile
// 重写 Module.prototype._compile
// 原因就不多写了，看下 Node 的源码就知道
Module.prototype._compile = function (content, filename) {
  if (filename.indexOf('app.asar') !== -1) {
    // 如果这个 JS 是在 app.asar 里面，就先解密
    return oldCompile.call(this, decrypt(Buffer.from(content, 'base64')), filename)
  }
  return oldCompile.call(this, content, filename)
}

try {
  require('./main.js') // 主进程创建窗口在这里面
} catch (err) {
  // 防止 Electron 报错后不退出
  dialog.showErrorBox(err.message, err.stack)
  process.exit(1)
}
```

这里有一个问题，C++ 里面拿不到 `require` 函数，因为 `require` 来自 JS Module 成员方法，原生模块没有 require 方法，怎么办哦？传呗，弄一个入口模块，缺什么传什么。稍微改一下上面的逻辑：

``` js
// 这是 C++
// 编译成 main.node
// 把上面的等价 JS 代码包一层
module.exports = function (thisArg, exports, require, module, __filename, __dirname) {
  // 上面的代码在这里面
}
```

对应 `node-addon-api` 的写法：

``` cpp
#include <napi.h>

// 必须要引用，因为 C++ 函数执行完以后所有局部变量栈内存会被回收
// 这样才能在其它回调函数中调用 crypto 库
static Napi::ObjectReference crypto; 

static Napi::Value run(const Napi::CallbackInfo& info) {
  // 判断参数类型
  // ...

  Napi::Function require = info[2].As<Napi::Function>();

  // 调用

  // ...
  crypto = Napi::Weak(require({ Napi::String::New(env, "crypto") }).As<Napi::Object>());
  // ...
  try {
    require({ Napi::String::New(env, "./main.js") });
  } catch (const Napi::Error& err) {
    // 弹窗后退出
  }
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  return Napi::Function::New(env, run);
}

// 不要分号，NODE_API_MODULE 是个宏
NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)
```

再来个入口文件 `index.js`：

``` js
// 这是真的 JS
require('./main.node')(this, exports, require, module, __filename, __dirname)
```

还记不记得上面说到 `index.js` 不要加密？因为这里必须最先跑，才能 Hack 掉 Node 的 API，实现解密操作，如果这里加密了，那连入口都跑不了。

总结下就是这样的：

1. 入口 `index.js` （未加密）里面 require `main.node` （已编译）
2. `main.node` （已编译） 里面 require `main.js` （已加密）
3. `main.js` （已加密）里面再 require 其它模块，创建窗口等等

## 渲染进程解密

为什么渲染进程不能和主进程一样 require `main.node` ？

因为渲染进程可能有很多个，就是说可能有很多个窗口，但是原生模块是不能被 Electron 渲染进程中的 Node 重复加载的，这个问题在 Electron 官方仓库有讨论，不建议在渲染进程加载原生模块。

那怎么搞？直接写成 JS 就可以了，因为渲染进程的 JS 也会被加密，不知道密钥没法解密，所以不用担心密钥泄露，把上面的 Hack `Module.prototype._compile` 的逻辑用 JS 写就行了。

这里有个局限，不能在 HTML 中直接引用 `<script>` 标签加载这个 JS，因为 HTML 中的 `<script>` 不走 `Module.prototype._compile`，所以只能在主进程中调用 `browserWindow.webContents.executeJavaScript()` 来为每个窗口最先运行这段代码，然后再 require 其它可能需要解密的 JS 文件。

## 局限性

* 只能加密 JS，不能加密其它类型的文件，如 JSON、图片资源等
* 如果 JS 文件很多，解密造成的性能影响较大
* 不能做成收费应用
* 不能算绝对安全，反编译原生模块仍然有密钥泄露和加密方法被得知的风险，只是相对于单纯的 ASAR 打包来说稍微提高了一点破解的门槛，源码不是那么容易被接触。如果有人真想蹂躏你的代码，这种方法防御力可能还远远不够。

最有效的方法是改 Electron 源码，重新编译 Electron。但是，动源码技术门槛高，重新编译 Electron 需要科学那什么，而且编译超慢。

## 总结

打包时做加密，运行时做解密，主进程放 C++ 里，渲染进程放加密的 JS 里。

最后关键，不要在预加载的代码中 `console.log`，不要忘了在生产环境关掉 `devTools` 和打开 `nodeIntegration`：

``` js
new BrowserWindow({
  // ...
  webPreferences: {
    nodeIntegration: true, // 渲染进程要使用 require
    devTools: false // 关掉开发者工具，因为开发者工具可以看到渲染进程的代码
  }
})
```
