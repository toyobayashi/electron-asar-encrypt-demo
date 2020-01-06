// TODO: Rewrite this script in C++ addon
const crypto = require('crypto')
const Module = require('module')

function decrypt (data) {
  const clearEncoding = 'utf8'
  const cipherEncoding = 'base64'
  const chunks = []
  const decipher = crypto.createDecipheriv('aes-256-cbc', '12345678123456781234567812345678', '1234567812345678')
  decipher.setAutoPadding(true)
  chunks.push(decipher.update(data, cipherEncoding, clearEncoding))
  chunks.push(decipher.final(clearEncoding))
  return chunks.join('')
}

const oldCompile = Module.prototype._compile

Module.prototype._compile = function (content, filename, ...args) {
  if (filename.indexOf('.asar') !== -1) {
    return oldCompile.call(this, decrypt(content, 'base64'), filename, ...args)
  }
  return oldCompile.call(this, content, filename, ...args)
}

try {
  require('./main.js')
} catch (err) {
  require('electron').dialog.showErrorBox(err.message, err.stack)
  process.exit(1)
}
