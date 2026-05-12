const binding = require('../binding')

module.exports = class ScanRecord {
  constructor(handle) {
    this._handle = handle
  }

  get serviceData() {
    return binding.scanRecordGetServiceData(this._handle)
  }
}
