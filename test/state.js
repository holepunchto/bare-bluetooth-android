const test = require('brittle')
const Central = require('../lib/central')
const Server = require('../lib/server')
const { isCI } = require('./helpers')

const WINDOW = 60000

function observe(emitter) {
  const seen = []

  return new Promise((resolve) => {
    const timer = setTimeout(() => resolve(seen), WINDOW)

    emitter.on('stateChange', (state) => {
      seen.push(state)

      if (seen.indexOf('off') !== -1 && seen.lastIndexOf('on') > seen.indexOf('off')) {
        clearTimeout(timer)
        resolve(seen)
      }
    })
  })
}

function check(t, name, seen) {
  t.comment(name + ': ' + seen.join(' -> '))

  t.ok(seen.indexOf('turningOff') !== -1, name + ' saw turningOff')
  t.ok(seen.indexOf('off') !== -1, name + ' saw off')
  t.ok(seen.indexOf('turningOn') > seen.indexOf('off'), name + ' saw turningOn after off')
  t.ok(seen.lastIndexOf('on') > seen.indexOf('off'), name + ' saw on again')
}

test('stateChange follows the radio off and on', { skip: isCI, timeout: WINDOW * 2 }, async (t) => {
  const central = new Central()
  t.teardown(() => central.destroy())

  const server = new Server()
  t.teardown(() => server.destroy())

  const initial = await Promise.all([
    new Promise((resolve) => central.once('stateChange', resolve)),
    new Promise((resolve) => server.once('stateChange', resolve))
  ])

  if (initial[0] !== 'on' || initial[1] !== 'on') {
    t.comment('bluetooth not on: ' + initial.join(', ') + ', skipping')
    return
  }

  t.comment('waiting up to ' + WINDOW / 1000 + 's for the radio to be toggled')

  const [fromCentral, fromServer] = await Promise.all([observe(central), observe(server)])

  if (fromCentral.indexOf('off') === -1 && fromServer.indexOf('off') === -1) {
    t.comment('no toggle observed, skipping - run: sh test/test-app/test.sh state')
    return
  }

  check(t, 'central', fromCentral)
  check(t, 'server', fromServer)
})
