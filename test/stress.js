const test = require('brittle')
const Central = require('../lib/central')
const Server = require('../lib/server')
const { isCI } = require('./helpers')

// Lifecycle stress: hammer init/destroy paths under allocation pressure so GC
// finalizers run interleaved with binding teardown. Reproduction harness for
// the peripherals-map corruption crash seen in the field; asserts survival,
// not behavior.

const ROUNDS = 150

function churn() {
  let keep = null
  for (let i = 0; i < 200; i++) keep = Buffer.alloc(65536, i & 0xff)
  return keep
}

function sleep(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms))
}

function state(emitter) {
  return new Promise((resolve) => emitter.once('stateChange', resolve))
}

test('stress: central init/destroy churn', { skip: isCI }, async (t) => {
  for (let i = 0; i < ROUNDS; i++) {
    const central = new Central()
    central.on('error', () => {})

    const s = await state(central)

    if (s !== 'on') {
      t.comment('bluetooth not on: ' + s + ', skipping')
      central.destroy()
      return
    }

    central.destroy()
    churn()
  }

  t.pass('survived ' + ROUNDS + ' central init/destroy rounds')
})

test('stress: server publish/unpublish churn', { skip: isCI }, async (t) => {
  for (let i = 0; i < ROUNDS; i++) {
    const server = new Server()
    server.on('error', () => {})

    const s = await state(server)

    if (s !== 'on') {
      t.comment('bluetooth not on: ' + s + ', skipping')
      server.destroy()
      return
    }

    server.publishChannel()

    const [psm] = await new Promise((resolve) => {
      server.once('channelPublish', (psm, error) => resolve([psm, error]))
    })

    if (psm > 0 && i % 2 === 0) server.unpublishChannel(psm)

    // Odd rounds leave the channel for destroy() to reclaim.
    server.destroy()
    churn()
  }

  t.pass('survived ' + ROUNDS + ' server publish/destroy rounds')
})

test(
  'stress: peripheral connect/destroy churn',
  { skip: isCI, timeout: 30 * 60 * 1000 },
  async (t) => {
    const central = new Central()
    central.on('error', () => {})

    const s = await state(central)

    if (s !== 'on') {
      t.comment('bluetooth not on: ' + s + ', skipping')
      central.destroy()
      return
    }

    central.startScan()

    const found = new Map()

    await new Promise((resolve) => {
      central.on('discover', (peripheral) => {
        found.set(peripheral.id, peripheral)
        if (found.size >= 5) resolve()
      })

      setTimeout(resolve, 10000)
    })

    central.stopScan()

    if (found.size === 0) {
      t.comment('no peripherals around, skipping connect stress')
      central.destroy()
      return
    }

    t.comment('discovered ' + found.size + ' peripherals')

    const targets = [...found.values()]

    for (const peripheral of targets) peripheral.on('error', () => {})

    for (let i = 0; i < ROUNDS; i++) {
      const peripheral = targets[i % targets.length]

      const settled = new Promise((resolve) => {
        central.once('connect', () => resolve('connect'))
        setTimeout(() => resolve('timeout'), 4000)
      })

      central.connect(peripheral)

      if (i % 3 === 0) {
        // Destroy while the connect may still be in flight.
        await sleep(0)
      } else {
        const outcome = await settled

        if (outcome === 'connect' && peripheral._handle) {
          // Poke the paths from the field crashes before tearing down.
          peripheral.requestMtu(517)
          peripheral.openL2CAPChannel(0x81)
          await sleep(50)
        }
      }

      central.disconnect(peripheral)
      peripheral.destroy()

      // Reset the JS wrapper so the next round re-attaches a fresh native peripheral.
      peripheral._destroyed = false

      churn()
    }

    central.destroy()
    t.pass('survived ' + ROUNDS + ' peripheral connect/destroy rounds')
  }
)
