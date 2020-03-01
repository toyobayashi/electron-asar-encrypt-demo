const fs = require('fs-extra')
const path = require('path')

console.log('Copying electron...')

fs.copySync(path.join(__dirname, 'node_modules/electron/dist'), path.join(__dirname, 'test'))

console.log('Copy electron done.')
