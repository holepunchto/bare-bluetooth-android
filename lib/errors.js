module.exports = class BluetoothError extends Error {
  constructor(msg, fn = BluetoothError, code = fn.name) {
    super(`${code}: ${msg}`)
    this.code = code

    if (Error.captureStackTrace) {
      Error.captureStackTrace(this, fn)
    }
  }

  get name() {
    return 'BluetoothError'
  }

  static ADVERTISE_FAILED(msg) {
    return new BluetoothError(msg, BluetoothError.ADVERTISE_FAILED)
  }

  static SCAN_FAILED(msg) {
    return new BluetoothError(msg, BluetoothError.SCAN_FAILED)
  }
}
