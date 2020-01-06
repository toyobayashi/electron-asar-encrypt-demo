try {
  require('./main.node')(this, module, exports, require, __dirname, __filename)
} catch (err) {
  require('electron').dialog.showErrorBox(err.message, err.stack)
  process.exit(1)
}

// try {
//   require('./main.js')
// } catch (err) {
//   require('electron').dialog.showErrorBox(err.message, err.stack)
//   process.exit(1)
// }
