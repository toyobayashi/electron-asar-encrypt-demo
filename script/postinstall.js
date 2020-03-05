const fs = require('fs-extra')
const getPath = require('./path.js')

console.log('Copying electron...')

fs.removeSync(getPath('test'))
fs.copySync(getPath('node_modules/electron/dist'), getPath('test'))

console.log('Copy electron done.')
