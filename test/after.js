module.exports = async function after({ device }, toggle) {
  toggle.stopped = true

  await toggle.done

  await device.shell('cmd', ['bluetooth_manager', 'enable'])

  if (toggle.error) throw toggle.error
}
