const binding = require('../binding')
const Device = require('./device')
const ScanRecord = require('./scan-record')

module.exports = class ScanResult {
  constructor(handle) {
    this._handle = handle
  }

  get device() {
    return new Device(binding.scanResultGetDevice(this._handle))
  }

  get rssi() {
    return binding.scanResultGetRssi(this._handle)
  }

  get scanRecord() {
    const handle = binding.scanResultGetScanRecord(this._handle)
    return handle ? new ScanRecord(handle) : null
  }
}
