const { createElement: e } = require('./element.js')
const msg = e('p', null, 'Message:')

function setMessage (message) {
  msg.innerHTML = `Message: ${message}`
}

module.exports = {
  element: msg,
  setMessage
}
