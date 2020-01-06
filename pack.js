const crypto = require('crypto')
const path = require('path')
const fs = require('fs')
const asar = require('asar')

asar.createPackageWithOptions(path.join(__dirname, './app'), path.join(__dirname, './test/resources/app.asar'), {
  unpack: '*.node',
  transform (filename) {
    if (path.extname(filename) === '.js' && path.basename(filename) !== 'index.js') {
      const hash = crypto.createHash('md5').update(fs.readFileSync(filename)).digest('hex')
      const iv = Buffer.from(hash, 'hex')
      var append = false
      var cipher = crypto.createCipheriv('aes-256-cbc', '0123456789abcdef0123456789abcdef', iv)
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

asar.createPackageWithOptions(path.join(__dirname, 'node_modules_asar'), path.join(__dirname, './test/resources/node_modules.asar'), {
  unpack: '*.node'
})
