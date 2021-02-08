for (let i = 0; i < process.argv.length; i++) {
  const arg = process.argv[i]
  if (arg.startsWith('--inspect') || arg.startsWith('--remote-debugging-port')) {
    throw new Error('Not allow debugging this program.')
  }
}
