const fs = require('fs-extra')
const getPath = require('./path.js')

console.log('Copying electron...')

fs.removeSync(getPath('test'))
fs.copySync(getPath('node_modules/electron/dist'), getPath('test'))

const asarTarget = process.platform === 'darwin' ? getPath(`test/Electron.app/Contents/Resources/default_app.asar`) : getPath('./test/resources/default_app.asar')
fs.removeSync(asarTarget)

console.log('Copy electron done.')
