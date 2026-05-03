const test = require('brittle')
const binding = require('../binding')
const Central = require('../lib/central')

const originals = {
  centralInit: binding.centralInit,
  centralStartScan: binding.centralStartScan,
  centralStopScan: binding.centralStopScan,
  centralConnect: binding.centralConnect,
  centralDisconnect: binding.centralDisconnect,
  centralDestroy: binding.centralDestroy
}

function setup() {
  let handlers = null

  binding.centralInit = (
    self,
    onstatechange,
    ondiscover,
    onconnect,
    ondisconnect,
    onconnectfail,
    onscanfail
  ) => {
    handlers = {
      self,
      onstatechange,
      ondiscover,
      onconnect,
      ondisconnect,
      onconnectfail,
      onscanfail
    }
    return { mock: true }
  }
  binding.centralStartScan = () => {}
  binding.centralStopScan = () => {}
  binding.centralConnect = () => {}
  binding.centralDisconnect = () => {}
  binding.centralDestroy = () => {}

  return {
    getHandlers: () => handlers,
    restore() {
      Object.assign(binding, originals)
    }
  }
}

function emitDiscover(ctx, id, rssi, serviceData) {
  ctx
    .getHandlers()
    .ondiscover.call(ctx.getHandlers().self, { mock: true }, id, 'mock-name', rssi, serviceData)
}

test('discovered peripheral carries serviceData when advertised', async (t) => {
  const ctx = setup()
  t.teardown(ctx.restore)

  const central = new Central()
  const serviceData = { '180d': new Uint8Array([0x01, 0x02, 0x03]) }

  const peripheral = await new Promise((resolve) => {
    central.on('discover', resolve)
    emitDiscover(ctx, 'p-1', -50, serviceData)
  })

  t.is(peripheral.serviceData, serviceData, 'serviceData reflects packet contents')
  t.is(peripheral.rssi, -50, 'rssi reflects packet contents')
})

test('discovered peripheral has null serviceData when not advertised', async (t) => {
  const ctx = setup()
  t.teardown(ctx.restore)

  const central = new Central()

  const peripheral = await new Promise((resolve) => {
    central.on('discover', resolve)
    emitDiscover(ctx, 'p-1', -60, null)
  })

  t.is(peripheral.serviceData, null, 'serviceData is null')
})

test('peripheral rssi and serviceData update in place across discover events', (t) => {
  const ctx = setup()
  t.teardown(ctx.restore)

  const central = new Central()
  const first = { '180d': new Uint8Array([0x01]) }
  const second = { '180d': new Uint8Array([0x02, 0x03]) }

  const seen = []
  central.on('discover', (peripheral) =>
    seen.push({
      peripheral,
      rssi: peripheral.rssi,
      serviceData: peripheral.serviceData
    })
  )

  emitDiscover(ctx, 'p-1', -40, first)
  emitDiscover(ctx, 'p-1', -50, second)
  emitDiscover(ctx, 'p-1', -60, null)

  t.is(seen.length, 3, 'discover fired three times')
  t.is(seen[0].peripheral, seen[1].peripheral, 'same peripheral reference across discoveries')
  t.is(seen[1].peripheral, seen[2].peripheral, 'same peripheral reference across discoveries')
  t.is(seen[0].rssi, -40, 'first emit captured first rssi')
  t.is(seen[0].serviceData, first, 'first emit captured first serviceData')
  t.is(seen[1].rssi, -50, 'second emit captured second rssi')
  t.is(seen[1].serviceData, second, 'second emit captured second serviceData')
  t.is(seen[2].rssi, -60, 'third emit captured third rssi')
  t.is(seen[2].serviceData, null, 'third emit captured null serviceData')
})

test('peripheral identity is stable across discover events', (t) => {
  const ctx = setup()
  t.teardown(ctx.restore)

  const central = new Central()

  const seen = []
  central.on('discover', (peripheral) => seen.push(peripheral))

  emitDiscover(ctx, 'p-1', -50, { '180d': new Uint8Array([0x01]) })
  emitDiscover(ctx, 'p-1', -55, null)

  t.is(seen[0], seen[1], 'same peripheral reference across discoveries')
})
