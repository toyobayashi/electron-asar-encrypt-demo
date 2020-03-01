const { ipcRenderer } = require('electron')
const { setMessage } = require('./message.js')
const { createElement: e } = require('./element.js')
const input = require('./input.js').getElementsByTagName('input')[0]

module.exports = e('button', {
  onClick () {
    const a = input.value.trim()
    let answer
    if ((/[^\\],/.test(a))) {
      const arr = a.split(',')
      for (let i = 0; i < arr.length; i++) {
        if (arr[i].endsWith('\\')) {
          arr[i] = arr[i].substring(0, arr[i].length - 1) + (arr[i + 1] || '')
          arr.splice(i + 1, 1)
        }
      }
      answer = arr.map(v => Number(v.trim()))
    } else {
      answer = a.replace(/\\/g, '').split('').map(v => v.charCodeAt(0))
    }

    const result = ipcRenderer.sendSync('check', answer)
    if (result.data) {
      setMessage('OK')
    } else {
      setMessage(result.err || 'Error')
    }
  }
}, 'Check')
