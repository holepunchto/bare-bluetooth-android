const os = require('bare-os')

exports.isCI = !!os.getEnv('CI')

exports.waitForOn = async function waitForOn(t, emitter) {
  const state = await new Promise((resolve) => {
    emitter.once('stateChange', resolve)
  })

  if (state === 'on') return true

  t.comment('bluetooth not on: ' + state + ', skipping')
  return false
}
