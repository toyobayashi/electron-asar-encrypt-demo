const { createElement: e } = require('./element.js')

module.exports = e('div', null, [
  e('p', null, `Electron: ${process.versions.electron}, Node.js: ${process.versions.node}`),
  e('p', null, 'Please enter the key. You can enter string or numbers.'),
  e('p', null, 'For example:'),
  e('p', null, 'secretkey'),
  e('p', null, '1, 2, 3, 4'),
  e('p', null, '0x01, 0x02, 0x03, 0x04'),
])
