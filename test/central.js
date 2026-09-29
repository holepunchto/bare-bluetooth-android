const test = require('brittle')
const Central = require('../lib/central')
const Peripheral = require('../lib/peripheral')
const ScanResult = require('../lib/scan-result')
const Device = require('../lib/device')
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

async function nearby(t, central) {
  central.on('error', () => {})

  const state = await new Promise((resolve) => {
    central.on('stateChange', resolve)
  })

  if (state !== 'on') {
    t.comment('bluetooth not on: ' + state + ', skipping')
    return null
  }

  central.startScan()

  const discovered = await new Promise((resolve) => {
    central.on('discover', resolve)

    setTimeout(() => resolve(null), 5000)
  })

  central.stopScan()

  if (discovered === null) t.comment('no peripheral advertising, skipping')

  return discovered
}

function settled(central) {
  return new Promise((resolve) => {
    central.once('connect', (peripheral) => resolve({ peripheral }))
    central.once('error', (error) => resolve({ error }))

    setTimeout(() => resolve({ timeout: true }), 5000)
  })
}

test('reconnect to the same peripheral after disconnect', { skip: isCI }, async (t) => {
  const central = new Central()
  t.teardown(() => central.destroy())

  const discovered = await nearby(t, central)
  if (discovered === null) return

  central.connect(discovered)

  const first = await settled(central)

  if (first.timeout || first.error) {
    t.comment('could not connect to nearby peripheral, skipping')
    return
  }

  central.disconnect(first.peripheral)

  central.connect(discovered)

  const second = await settled(central)

  t.absent(second.timeout, 'reconnect did not time out')
  t.absent(second.error, 'reconnect did not error')
  t.ok(second.peripheral, 'reconnect emits connect again')
})

test('redial after abandoning a pending dial', { skip: isCI }, async (t) => {
  const central = new Central()
  t.teardown(() => central.destroy())

  const discovered = await nearby(t, central)
  if (discovered === null) return

  central.connect(discovered)
  central.disconnect(discovered)

  central.connect(discovered)

  const result = await settled(central)

  if (result.timeout || result.error) {
    t.comment('could not connect to nearby peripheral, skipping')
    return
  }

  t.ok(result.peripheral, 'redial emits connect')
  t.is(result.peripheral, discovered, 'connect carries the redialled peripheral')
})

test('disconnect of an earlier connection leaves a pending dial alone', { skip: isCI }, (t) => {
  const central = new Central()
  t.teardown(() => central.destroy())

  const id = '00:11:22:33:44:55'
  const device = new Device({ address: id })
  const pending = new Peripheral({ scanResult: new ScanResult({ device, rssi: -50 }) })
  central._connected.set(id, pending)

  central.on('error', () => t.fail('error emitted for an earlier connection'))
  central.on('disconnect', () => t.fail('disconnect emitted for an earlier connection'))

  t.execution(() => central._ondisconnect(id, 'GATT error 133'))
  t.is(central._connected.get(id), pending, 'the pending dial is still tracked')
})

test('disconnect of the current connection is reported', { skip: isCI }, (t) => {
  const central = new Central()
  t.teardown(() => central.destroy())

  const id = '00:11:22:33:44:55'
  let reported = null
  // TODO: a fake peripheral because _ondisconnect calls straight into the
  // binding. Pull the "is this my connection" decision out of the native path
  // so this can be tested with plain data.
  const connected = {
    id,
    _handle: {},
    _ondisconnect(error) {
      reported = error
    },
    destroy() {}
  }
  central._connected.set(id, connected)

  let emitted = null
  central.on('error', (err) => {
    emitted = err
  })

  central._ondisconnect(id, 'GATT error 133')

  t.is(reported, 'GATT error 133', 'the peripheral is told')
  t.is(emitted.code, 'DISCONNECT')
  t.absent(central._connected.has(id))
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

    setTimeout(() => resolve(null), 10000)
  })

  central.stopScan()

  if (peripheral === null) {
    t.comment('no peripheral advertising, skipping')
    return
  }

  t.ok(peripheral.scanResult, 'peripheral has scanResult')
  t.ok(typeof peripheral.id === 'string', 'peripheral has string id')
  t.ok(peripheral.id.length > 0, 'peripheral id is non-empty')
  t.is(peripheral.id, peripheral.scanResult.device.address, 'id is the device address')
  t.ok(typeof peripheral.rssi === 'number', 'peripheral has numeric rssi')
  t.ok(peripheral.rssi < 0, 'rssi is negative')
  t.ok(peripheral.name === null || typeof peripheral.name === 'string', 'name is string or null')
  t.ok(
    peripheral.scanResult.scanRecord === null ||
      typeof peripheral.scanResult.scanRecord === 'object',
    'scanRecord is object or null'
  )
  t.ok(
    peripheral.serviceData === null || typeof peripheral.serviceData === 'object',
    'serviceData is object or null'
  )
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

    setTimeout(() => resolve({ timeout: true }), 10000)
  })

  central.stopScan()

  if (result.timeout) {
    t.comment('no peripheral advertised twice, skipping')
    return
  }

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

test('scan with a malformed service UUID throws', { skip: isCI }, (t) => {
  const central = new Central()
  t.teardown(() => central.destroy())

  t.exception(() => central.startScan(['not-a-uuid']), /IllegalArgumentException/)
})

test('a malformed service UUID leaves the binding usable', { skip: isCI }, (t) => {
  const central = new Central()
  t.teardown(() => central.destroy())

  try {
    central.startScan(['not-a-uuid'])
  } catch {}

  // exception cleared
  t.exception(() => central.startScan(['still-not-a-uuid']), /IllegalArgumentException/)
})

test('scan with the radio off throws', { skip: isCI }, async (t) => {
  const central = new Central()
  t.teardown(() => central.destroy())

  const state = await new Promise((resolve) => {
    central.on('stateChange', resolve)
  })

  if (state !== 'off') {
    t.comment('bluetooth on: ' + state + ', skipping')
    return
  }

  t.exception(() => central.startScan(), /scanner unavailable/)
})
