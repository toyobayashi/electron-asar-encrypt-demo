for (let i = 0; i < process.argv.length; i++) {
  const arg = process.argv[i]
  if (arg && arg.indexOf('--inspect') !== -1 || arg.indexOf('--remote-debugging-port') !== -1) {
    throw new Error('Not allow debugging this program.')
  }
}
