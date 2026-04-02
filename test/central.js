const test = require('brittle')
const Central = require('../lib/central')
const { isCI } = require('./helpers')

test('central emits stateChange on init', { skip: isCI }, async (t) => {
  const central = new Central()
  t.teardown(() => central.destroy())

  const state = await new Promise((resolve) => {
    central.on('stateChange', resolve)
  })

  t.ok(typeof state === 'string', 'state is a string')
  t.ok(['on', 'off', 'turningOn', 'turningOff'].includes(state), 'state is a valid value: ' + state)
})

test('central tracks state property', { skip: isCI }, async (t) => {
  const central = new Central()
  t.teardown(() => central.destroy())

  t.is(central.state, 'off', 'initial state is off')

  const state = await new Promise((resolve) => {
    central.on('stateChange', resolve)
  })

  t.is(central.state, state, 'state property matches emitted state')
})

test('central exports state constants', (t) => {
  t.is(Central.STATE_OFF, 10)
  t.is(Central.STATE_TURNING_ON, 11)
  t.is(Central.STATE_ON, 12)
  t.is(Central.STATE_TURNING_OFF, 13)
})

test('scan discovers peripherals with expected shape', { skip: isCI }, async (t) => {
  const central = new Central()
  t.teardown(() => central.destroy())

  const state = await new Promise((resolve) => {
    central.on('stateChange', resolve)
  })

  if (state !== 'on') {
    t.comment('bluetooth not on: ' + state + ', skipping')
    return
  }

  central.startScan()

  const peripheral = await new Promise((resolve) => {
    central.on('discover', resolve)
  })

  central.stopScan()

  t.ok(peripheral.handle, 'peripheral has handle')
  t.ok(typeof peripheral.id === 'string', 'peripheral has string id')
  t.ok(peripheral.id.length > 0, 'peripheral id is non-empty')
  t.ok(typeof peripheral.rssi === 'number', 'peripheral has numeric rssi')
  t.ok(peripheral.rssi < 0, 'rssi is negative')
  t.ok(peripheral.name === null || typeof peripheral.name === 'string', 'name is string or null')
})

test('scan deduplicates peripherals by id', { skip: isCI }, async (t) => {
  const central = new Central()
  t.teardown(() => central.destroy())

  const state = await new Promise((resolve) => {
    central.on('stateChange', resolve)
  })

  if (state !== 'on') {
    t.comment('bluetooth not on, skipping')
    return
  }

  central.startScan()

  const result = await new Promise((resolve) => {
    const seen = new Map()
    let count = 0

    central.on('discover', (peripheral) => {
      if (seen.has(peripheral.id)) {
        resolve({ same: peripheral === seen.get(peripheral.id) })
        return
      }

      seen.set(peripheral.id, peripheral)
      count++

      if (count > 100) {
        resolve({ same: false })
      }
    })
  })

  central.stopScan()

  t.ok(result.same, 'same object reference for duplicate peripheral id')
})

test('filtered scan with non-existent service UUID finds nothing', { skip: isCI }, async (t) => {
  const central = new Central()
  t.teardown(() => central.destroy())

  const state = await new Promise((resolve) => {
    central.on('stateChange', resolve)
  })

  if (state !== 'on') {
    t.comment('bluetooth not on, skipping')
    return
  }

  central.startScan(['00000000-0000-0000-0000-000000000000'])

  let found = false

  central.on('discover', () => {
    found = true
  })

  await new Promise((resolve) => setTimeout(resolve, 3000))

  central.stopScan()

  t.absent(found, 'no peripherals discovered with non-existent service UUID')
})
