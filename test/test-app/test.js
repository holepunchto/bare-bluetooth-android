/* global BareKit */
const FramedStream = require('framed-stream')

const stream = new FramedStream(BareKit.IPC)
const log = console.log.bind(console)

// The IPC queue is bounded, so lines wait their turn rather than overflowing it.
const queue = []
let waiting = false

function flush() {
  while (queue.length > 0) {
    if (stream.write(queue.shift()) === false) {
      waiting = true
      stream.once('drain', () => {
        waiting = false
        flush()
      })
      return
    }
  }
}

// brittle binds console.log when it loads, so patch it before requiring the suite.
console.log = (...args) => {
  log(...args)
  queue.push(Buffer.from(args.map(String).join(' ') + '\n'))
  if (!waiting) flush()
}

// The app sends one frame once it is reading, so no output is written into the void.
stream.once('data', () => require('../../test.js'))
