const { render } = require('./element.js')

render(document.body, [
  require('./title.js'),
  require('./input.js'),
  require('./message.js').element,
  require('./button.js')
])
