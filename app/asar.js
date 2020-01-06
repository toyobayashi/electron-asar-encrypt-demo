// TODO: Rewrite this script in C++ addon
const crypto = require('crypto')
const path = require('path')
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
  if (filename.indexOf('app.asar') !== -1) {
    return oldCompile.call(this, decrypt(content, 'base64'), filename, ...args)
  }
  return oldCompile.call(this, content, filename, ...args)
}

const originalResolveLookupPaths = Module._resolveLookupPaths

Module._resolveLookupPaths = originalResolveLookupPaths.length === 2 ? function (request, parent) {
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

try {
  require('./main.js')
} catch (err) {
  require('electron').dialog.showErrorBox(err.message, err.stack)
  process.exit(1)
}
