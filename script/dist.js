const cz = require('@tybys/cross-zip')
const getPath = require('./path.js')
cz.zipSync(getPath('test'), getPath(`dist/electron-encrypted-${process.platform}-${process.arch}.zip`))
