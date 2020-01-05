const crypto = require('crypto')
const path = require('path')
const asar = require('asar')

asar.createPackageWithOptions(path.join(__dirname, './app'), path.join(__dirname, './test/resources/app.asar'), {
  unpack: 'asar.js',
  transform (filename) {
    if (path.extname(filename) === '.js') {
      var cipher = crypto.createCipheriv('aes-256-cbc', '12345678123456781234567812345678', '1234567812345678')
      cipher.setAutoPadding(true)
      return cipher
    }
  }
})
