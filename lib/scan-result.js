module.exports = class ScanResult {
  constructor({ device, rssi, scanRecord = null }) {
    this._device = device
    this._rssi = rssi
    this._scanRecord = scanRecord
  }

  get device() {
    return this._device
  }

  get rssi() {
    return this._rssi
  }

  get scanRecord() {
    return this._scanRecord
  }
}
