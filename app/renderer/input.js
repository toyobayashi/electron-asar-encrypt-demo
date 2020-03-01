const { createElement: e } = require('./element.js')

module.exports = e('div', { style: 'width: 400px' }, [
  e('input', { style: 'width: 100%; box-sizing: border-box;' })
])
