const childProcess = require('child_process')
const getPath = require('./path.js')

const asarTarget = process.platform === 'darwin'
  ? getPath(`test/Electron.app/Contents/MacOS/Electron`)
  : getPath(`./test/electron${process.platform === 'win32' ? '.exe' : ''}`)

childProcess.spawn(asarTarget, { stdio: 'ignore', detached: true }).unref()
