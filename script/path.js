module.exports = function (...args) {
  return require('path').join(__dirname, '..', ...args)  
}
