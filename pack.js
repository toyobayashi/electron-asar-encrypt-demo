const crypto = require('crypto')
const path = require('path')
const fs = require('fs')
const asar = require('asar')

const key = Buffer.from(fs.readFileSync(path.join(__dirname, 'src/key.txt'), 'utf8').trim().split(',').map(v => Number(v.trim())))

asar.createPackageWithOptions(path.join(__dirname, './app'), path.join(__dirname, './test/resources/app.asar'), {
  unpack: '*.node',
  transform (filename) {
    if (path.extname(filename) === '.js' && path.basename(filename) !== 'hack.js') {
      const iv = crypto.randomBytes(16)
      var append = false
      var cipher = crypto.createCipheriv('aes-256-cbc', key, iv)
      cipher.setAutoPadding(true)
      cipher.setEncoding('base64')

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
})

const target = process.platform === 'darwin' ? path.join(__dirname, `test/Electron.app/Contents/Resources/node_modules.asar`) : path.join(__dirname, './test/resources/node_modules.asar')
asar.createPackageWithOptions(path.join(__dirname, 'node_modules_asar'), target, {
  unpack: '*.node'
})
