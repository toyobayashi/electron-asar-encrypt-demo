const fs = require('fs')
const path = require('path')
const crypto = require('crypto')

const target = path.join(__dirname, 'src/key.txt')

if (!fs.existsSync(target)) {
  writeKey()
} else {
  if (process.argv.slice(2)[0]) {
    writeKey()
  }
}

function writeKey () {
  fs.writeFileSync(target, Array.prototype.map.call(crypto.randomBytes(32), (v => ('0x' + ('0' + v.toString(16)).slice(-2)))))
}
