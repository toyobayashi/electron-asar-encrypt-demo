// TODO: Rewrite this script in C++ addon

const crypto = require('crypto')
const path = require('path')
const fs = require('fs')

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
const oldReadFileSync = fs.readFileSync
fs.readFileSync = function (...args) {
  if (args[0].indexOf('.asar') !== -1 && path.extname(args[0]) === '.js') {
    const data = oldReadFileSync.call(this, args[0], 'binary')
    return decode(data)
  }
  const data = oldReadFileSync.call(this, ...args)
  return data
}
console.log('readFileSync hacked')
require('{{entry}}')
