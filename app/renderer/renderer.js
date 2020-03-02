console.log(module.parent === window.module)
require('../asar.js')
console.warn(require('outerpkg'))

const { render } = require('./element.js')

render(document.body, [
  require('./title.js'),
  require('./input.js'),
  require('./message.js').element,
  require('./button.js')
])
