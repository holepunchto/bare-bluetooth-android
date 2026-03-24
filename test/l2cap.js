const test = require('brittle')
const Server = require('../lib/server')
const { isCI } = require('./helpers')

test('server publish L2CAP channel returns PSM', { skip: isCI }, async (t) => {
  const server = new Server()
  t.teardown(() => server.destroy())

  const state = await new Promise((resolve) => {
    server.on('stateChange', resolve)
  })

  if (state !== 'on') {
    t.comment('bluetooth not on: ' + state + ', skipping')
    return
  }

  server.publishChannel()

  const [psm, error] = await new Promise((resolve) => {
    server.on('channelPublish', (psm, error) => {
      resolve([psm, error])
    })
  })

  t.absent(error, 'no error publishing channel')
  t.ok(typeof psm === 'number', 'psm is a number')
  t.ok(psm > 0, 'psm is positive: ' + psm)

  server.unpublishChannel(psm)
})

test('server publish multiple L2CAP channels', { skip: isCI }, async (t) => {
  const server = new Server()
  t.teardown(() => server.destroy())

  const state = await new Promise((resolve) => {
    server.on('stateChange', resolve)
  })

  if (state !== 'on') {
    t.comment('bluetooth not on: ' + state + ', skipping')
    return
  }

  server.publishChannel()

  const [psm1, error1] = await new Promise((resolve) => {
    server.on('channelPublish', (psm, error) => {
      resolve([psm, error])
    })
  })

  t.absent(error1, 'no error publishing first channel')
  t.ok(psm1 > 0, 'first psm is positive: ' + psm1)

  server.publishChannel()

  const [psm2, error2] = await new Promise((resolve) => {
    server.once('channelPublish', (psm, error) => {
      resolve([psm, error])
    })
  })

  t.absent(error2, 'no error publishing second channel')
  t.ok(psm2 > 0, 'second psm is positive: ' + psm2)
  t.not(psm1, psm2, 'psms are different')

  server.unpublishChannel(psm1)
  server.unpublishChannel(psm2)
})

test('server publish encrypted L2CAP channel', { skip: isCI }, async (t) => {
  const server = new Server()
  t.teardown(() => server.destroy())

  const state = await new Promise((resolve) => {
    server.on('stateChange', resolve)
  })

  if (state !== 'on') {
    t.comment('bluetooth not on: ' + state + ', skipping')
    return
  }

  server.publishChannel({ encrypted: true })

  const [psm, error] = await new Promise((resolve) => {
    server.on('channelPublish', (psm, error) => {
      resolve([psm, error])
    })
  })

  t.absent(error, 'no error publishing encrypted channel')
  t.ok(psm > 0, 'encrypted channel psm is positive: ' + psm)

  server.unpublishChannel(psm)
})
