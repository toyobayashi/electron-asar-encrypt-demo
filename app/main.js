require('./check.js')
require('./asar.js')
const str = require('outerpkg')
const WindowManager = require('./win.js')
const { app, ipcMain, Menu } = require('electron')

const assert = require('assert')

if (!process.env.ELECTRON_RUN_AS_NODE) {
  console.log(`${process.type}: ${str}`)
} else {
  assert.strictEqual(str, 'outerpkg export string', 'Failed to load node_modules.asar.')
}

if (!process.env.ELECTRON_RUN_AS_NODE) {
  app.on('window-all-closed', function () {
    if (process.platform !== 'darwin') {
      app.quit()
    }
  })
  
  app.on('activate', function () {
    WindowManager.createMainWindow()
  })
}


function mustNotExportKey (key) {
  ipcMain.on('check', (e, arr) => {
    if (arr.length !== key.length) {
      e.returnValue = {
        err: null,
        data: false
      }
      return
    }
    for (let i = 0; i < key.length; i++) {
      try {
        if (key[i] !== arr[i]) {
          e.returnValue = {
            err: null,
            data: false
          }
          return
        }
      } catch (e) {
        e.returnValue = {
          err: e.message,
          data: false
        }
        return
      }
    }
    e.returnValue = {
      err: null,
      data: true
    }
  })
}

function main () {
  if (process.platform === 'darwin') {
    const template = [
      {
        label: app.name,
        submenu: [
          { role: 'about' },
          { type: 'separator' },
          { role: 'services' },
          { type: 'separator' },
          { role: 'hide' },
          { role: 'hideothers' },
          { role: 'unhide' },
          { type: 'separator' },
          { role: 'quit' }
        ]
      }
    ]

    const menu = Menu.buildFromTemplate(template)
    Menu.setApplicationMenu(menu)
  }
  WindowManager.createMainWindow()
  // WindowManager.getInstance().createWindow('another-window', {
  //   width: 800,
  //   height: 600,
  //   show: false,
  //   webPreferences: {
  //     nodeIntegration: true,
  //     devTools: false
  //   }
  // },
  // require('url').format({
  //   pathname: require('path').join(__dirname, './index.html'),
  //   protocol: 'file:',
  //   slashes: true
  // }),
  // './renderer/renderer.js')
}

module.exports = function bootstrap (k) {
  if (!Array.isArray(k) || k.length === 0) {
    throw new Error('Failed to bootstrap application.')
  }
  WindowManager.__SECRET_KEY__ = k
  
  if (!process.env.ELECTRON_RUN_AS_NODE) {
    mustNotExportKey(k)
    if (app.whenReady === 'function') {
      app.whenReady().then(main).catch(err => console.log(err))
    } else {
      app.on('ready', main)
    }
  } else {
    console.log(k)
    assert.strictEqual(k.length, 32, 'Key length error.')
    console.log('\nTest passed.\n')
  }
}
