module.exports = class ScanRecord {
  constructor({ serviceData }) {
    this._serviceData = serviceData === undefined ? null : serviceData
  }

  get serviceData() {
    return this._serviceData
  }
}
