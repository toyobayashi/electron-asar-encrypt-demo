const cz = require('@tybys/cross-zip')
const getPath = require('./path.js')
require('fs').copyFileSync(getPath('./src/key.txt'), getPath(`test/key-${process.platform}-${process.arch}.txt`))
cz.zipSync(getPath('test'), getPath(`dist/electron-encrypted-${process.platform}-${process.arch}.zip`))
