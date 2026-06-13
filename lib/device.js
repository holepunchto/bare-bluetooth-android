module.exports = class Device {
  constructor({ address, name = null }) {
    this._address = address
    this._name = name
  }

  get address() {
    return this._address
  }

  get name() {
    return this._name
  }
}
