const binding = require('../binding')

module.exports = exports = class Device {
  constructor({ address, name = null, type = null }) {
    this._address = address
    this._name = name
    this._type = type
  }

  get address() {
    return this._address
  }

  get name() {
    return this._name
  }

  get type() {
    return this._type
  }
}

exports.DEVICE_TYPE_UNKNOWN = binding.DEVICE_TYPE_UNKNOWN
exports.DEVICE_TYPE_CLASSIC = binding.DEVICE_TYPE_CLASSIC
exports.DEVICE_TYPE_LE = binding.DEVICE_TYPE_LE
exports.DEVICE_TYPE_DUAL = binding.DEVICE_TYPE_DUAL
