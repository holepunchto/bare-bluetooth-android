const binding = require('../binding')
const Device = require('./device')
const ScanRecord = require('./scan-record')

module.exports = class ScanResult {
  constructor(handle) {
    this._handle = handle
    this._device = new Device(binding.scanResultGetDevice(handle))

    const scanRecordHandle = binding.scanResultGetScanRecord(handle)
    this._scanRecord = scanRecordHandle ? new ScanRecord(scanRecordHandle) : null
  }

  get device() {
    return this._device
  }

  get rssi() {
    return binding.scanResultGetRssi(this._handle)
  }

  get scanRecord() {
    return this._scanRecord
  }
}
