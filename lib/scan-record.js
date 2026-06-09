module.exports = class ScanRecord {
  constructor({ serviceData = null }) {
    this._serviceData = serviceData
  }

  get serviceData() {
    return this._serviceData
  }
}
