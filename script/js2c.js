const fs = require('fs')
const getPath = require('./path.js')
let terser
try {
  terser = require('terser')
} catch (_) {
  terser = {
    minify (code) {
      return { code }
    }
  }
}

var options = {}

var result1 = terser.minify(fs.readFileSync(getPath('src/find.js'), 'utf8'), options);
var result2 = terser.minify(fs.readFileSync(getPath('src/require.js'), 'utf8'), options);

function wrap (code) {
  return `(${code});`
}

function str2buf (str) {
  const zero = Buffer.alloc(1)
  zero[0] = 0
  return Buffer.concat([Buffer.from(str), zero])
}

function buf2pchar(buf, varname) {
  return `const char ${varname}[]={${Array.prototype.join.call(buf, ',')}};`
}

const scriptFind = buf2pchar(str2buf(wrap(result1.code)), 'scriptFind')
const scriptRequire = buf2pchar(str2buf(wrap(result2.code)), 'scriptRequire')

fs.writeFileSync(getPath('src/script.h'), scriptFind + '\n' + scriptRequire + '\n', 'utf8')
