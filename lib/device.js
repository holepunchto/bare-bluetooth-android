const binding = require('../binding')

module.exports = class Device {
  constructor(handle) {
    this._handle = handle
  }

  get address() {
    return binding.deviceGetAddress(this._handle)
  }

  get name() {
    return binding.deviceGetName(this._handle)
  }
}
