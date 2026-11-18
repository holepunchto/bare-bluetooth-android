module.exports = class ScanRecord {
  constructor({ serviceData = null, serviceUUIDs = [] } = {}) {
    this._serviceData = serviceData
    this._serviceUUIDs = serviceUUIDs
  }

  get serviceData() {
    return this._serviceData
  }

  get serviceUUIDs() {
    return this._serviceUUIDs
  }
}
