const POLL = 2000
const OFF = 8000

module.exports = async function before({ device }) {
  const since = (await device.shell('date', ['+%s.%N'])).trim()

  const toggle = { stopped: false, error: null }

  toggle.done = (async () => {
    while (!toggle.stopped) {
      const log = await device.shell('logcat', ['-d', '-v', 'raw', '-T', since])

      if (log.includes('radio to be toggled')) break

      await new Promise((resolve) => setTimeout(resolve, POLL))
    }

    if (toggle.stopped) return

    await device.shell('cmd', ['bluetooth_manager', 'disable'])

    await new Promise((resolve) => setTimeout(resolve, OFF))

    await device.shell('cmd', ['bluetooth_manager', 'enable'])
  })().catch((err) => {
    toggle.error = err
  })

  return toggle
}
