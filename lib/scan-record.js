module.exports = class ScanRecord {
  constructor(opts) {
    this._serviceData = opts.serviceData
  }

  get serviceData() {
    return this._serviceData
  }
}
