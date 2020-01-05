// TODO: Rewrite this script in C++ addon

const oldReadFileSync = require('fs').readFileSync
const path = require('path')
const crypto = require('crypto')
const dialog = require('electron').dialog

const decode = (data) => {
  var clearEncoding = 'utf8'
  var cipherEncoding = 'binary'
  var cipherChunks = []
  var decipher = crypto.createDecipheriv('aes-256-cbc', '12345678123456781234567812345678', '1234567812345678')
  decipher.setAutoPadding(true)
  cipherChunks.push(decipher.update(data, cipherEncoding, clearEncoding))
  cipherChunks.push(decipher.final(clearEncoding))
  return cipherChunks.join('')
}

require('fs').readFileSync = function (...args) {
  if (args[0].indexOf('.asar') !== -1 && path.extname(args[0]) === '.js') {
    const data = oldReadFileSync.call(this, args[0])
    return decode(data)
  }
  const data = oldReadFileSync.call(this, ...args)
  return data
}

try {
  require(path.join(__dirname, './main.js'))
} catch (err) {
  dialog.showErrorBox(err.message, err.stack)
  process.exit(1)
}
