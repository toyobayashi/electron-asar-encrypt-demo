require('./main.node').run(this, module, exports, require, __dirname, __filename)

try {
  require('./main.js')
} catch (err) {
  require('electron').dialog.showErrorBox(err.message, err.stack)
  process.exit(1)
}
