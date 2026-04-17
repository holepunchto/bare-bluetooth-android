const test = require('brittle')
const Server = require('../lib/server')
const Service = require('../lib/service')
const Characteristic = require('../lib/characteristic')
const { isCI } = require('./helpers')

const SERVICE_UUID = '12345678-1234-1234-1234-123456789ABC'
const CHAR_UUID = '87654321-4321-4321-4321-CBA987654321'

test('server emits stateChange on init', { skip: isCI }, async (t) => {
  const server = new Server()
  t.teardown(() => server.destroy())

  const state = await new Promise((resolve) => {
    server.on('stateChange', resolve)
  })

  t.ok(typeof state === 'string', 'state is a string')
  t.ok(['on', 'off', 'turningOn', 'turningOff'].includes(state), 'state is a valid value: ' + state)
})

test('server tracks state property', { skip: isCI }, async (t) => {
  const server = new Server()
  t.teardown(() => server.destroy())

  t.is(server.state, 'off', 'initial state is off')

  const state = await new Promise((resolve) => {
    server.on('stateChange', resolve)
  })

  t.is(server.state, state, 'state property matches emitted state')
})

test('server exports state constants', (t) => {
  t.is(Server.STATE_OFF, 10)
  t.is(Server.STATE_TURNING_ON, 11)
  t.is(Server.STATE_ON, 12)
  t.is(Server.STATE_TURNING_OFF, 13)
})

test('server exports permission constants', (t) => {
  t.is(Server.PERMISSION_READABLE, 0x01)
  t.is(Server.PERMISSION_WRITEABLE, 0x02)
  t.is(Server.PERMISSION_READ_ENCRYPTED, 0x04)
  t.is(Server.PERMISSION_WRITE_ENCRYPTED, 0x08)
})

test('server exports ATT result constants', (t) => {
  t.is(Server.ATT_SUCCESS, 0x00)
  t.is(Server.ATT_INVALID_HANDLE, 0x01)
  t.is(Server.ATT_READ_NOT_PERMITTED, 0x02)
  t.is(Server.ATT_WRITE_NOT_PERMITTED, 0x03)
  t.is(Server.ATT_INSUFFICIENT_RESOURCES, 0x11)
  t.is(Server.ATT_UNLIKELY_ERROR, 0x0e)
})

test('server add service with static value', { skip: isCI }, async (t) => {
  const server = new Server()
  t.teardown(() => server.destroy())

  const state = await new Promise((resolve) => {
    server.on('stateChange', resolve)
  })

  if (state !== 'on') {
    t.comment('bluetooth not on: ' + state + ', skipping')
    return
  }

  const characteristic = new Characteristic(CHAR_UUID, {
    read: true,
    value: new Uint8Array([0x48, 0x65, 0x6c, 0x6c, 0x6f])
  })

  const service = new Service(SERVICE_UUID, [characteristic])

  server.addService(service)

  const [uuid, error] = await new Promise((resolve) => {
    server.on('serviceAdd', (uuid, error) => {
      resolve([uuid, error])
    })
  })

  t.absent(error, 'no error adding service')
  t.is(uuid, SERVICE_UUID, 'service uuid matches')
})

test('server add service with dynamic characteristic', { skip: isCI }, async (t) => {
  const server = new Server()
  t.teardown(() => server.destroy())

  const state = await new Promise((resolve) => {
    server.on('stateChange', resolve)
  })

  if (state !== 'on') {
    t.comment('bluetooth not on: ' + state + ', skipping')
    return
  }

  const dynamicUuid = 'AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE'

  const characteristic = new Characteristic(dynamicUuid, {
    read: true,
    write: true
  })

  const serviceUuid = 'BBBBBBBB-CCCC-DDDD-EEEE-FFFFFFFFFFFF'
  const service = new Service(serviceUuid, [characteristic])

  server.addService(service)

  const [uuid, error] = await new Promise((resolve) => {
    server.on('serviceAdd', (uuid, error) => {
      resolve([uuid, error])
    })
  })

  t.absent(error, 'no error adding dynamic service')
  t.is(uuid, serviceUuid, 'dynamic service uuid matches')
})

test('server start and stop advertising', { skip: isCI }, async (t) => {
  const server = new Server()
  t.teardown(() => server.destroy())

  const state = await new Promise((resolve) => {
    server.on('stateChange', resolve)
  })

  if (state !== 'on') {
    t.comment('bluetooth not on: ' + state + ', skipping')
    return
  }

  const characteristic = new Characteristic(CHAR_UUID, {
    read: true,
    value: new Uint8Array([0x01])
  })

  const service = new Service(SERVICE_UUID, [characteristic])

  server.addService(service)

  await new Promise((resolve) => {
    server.on('serviceAdd', () => resolve())
  })

  t.execution(() => {
    server.startAdvertising({
      name: 'BareTestAdv',
      serviceUUIDs: [SERVICE_UUID]
    })
  })

  t.execution(() => {
    server.stopAdvertising()
  })
})

test('server multiple characteristics in one service', { skip: isCI }, async (t) => {
  const server = new Server()
  t.teardown(() => server.destroy())

  const state = await new Promise((resolve) => {
    server.on('stateChange', resolve)
  })

  if (state !== 'on') {
    t.comment('bluetooth not on: ' + state + ', skipping')
    return
  }

  const charA = new Characteristic('11111111-1111-1111-1111-111111111111', {
    read: true,
    value: new Uint8Array([0x01])
  })

  const charB = new Characteristic('22222222-2222-2222-2222-222222222222', {
    read: true,
    write: true,
    notify: true
  })

  const serviceUuid = 'CCCCCCCC-CCCC-CCCC-CCCC-CCCCCCCCCCCC'
  const service = new Service(serviceUuid, [charA, charB])

  server.addService(service)

  const [uuid, error] = await new Promise((resolve) => {
    server.on('serviceAdd', (uuid, error) => {
      resolve([uuid, error])
    })
  })

  t.absent(error, 'no error adding multi-characteristic service')
  t.is(uuid, serviceUuid, 'multi-characteristic service uuid matches')
})
