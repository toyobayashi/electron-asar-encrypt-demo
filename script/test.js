require('asar-node/lib/autorun/index.js')

const getPath = require('./path.js')

process.env.ELECTRON_RUN_AS_NODE = '1'
const asarTarget = process.platform === 'darwin' ? getPath(`test/Electron.app/Contents/Resources/app.asar`) : getPath('./test/resources/app.asar')
require('module')._load(asarTarget, null, true)
