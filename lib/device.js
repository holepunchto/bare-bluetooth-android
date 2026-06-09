module.exports = class Device {
  constructor({ address, name }) {
    this._address = address
    this._name = name === undefined ? null : name
  }

  get address() {
    return this._address
  }

  get name() {
    return this._name
  }
}
