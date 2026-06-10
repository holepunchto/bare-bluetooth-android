const EventEmitter = require('bare-events')
const binding = require('../binding')
const Peripheral = require('./peripheral')
const Device = require('./device')
const ScanRecord = require('./scan-record')
const ScanResult = require('./scan-result')

const STATES = {
  [binding.STATE_OFF]: 'off',
  [binding.STATE_TURNING_ON]: 'turningOn',
  [binding.STATE_ON]: 'on',
  [binding.STATE_TURNING_OFF]: 'turningOff'
}

module.exports = exports = class Central extends EventEmitter {
  constructor() {
    super()

    this._handle = binding.centralInit(
      this,
      this._onstatechange,
      this._ondiscover,
      this._onconnect,
      this._ondisconnect,
      this._onconnectfail,
      this._onscanfail
    )

    this._peripherals = new Map()
    this._connected = new Map()
    this._state = 'off'
  }

  get state() {
    return this._state
  }

  startScan(serviceUUIDs, opts = {}) {
    const uuids = serviceUUIDs ? serviceUUIDs.map((s) => binding.createUUID(s)) : undefined
    binding.centralStartScan(this._handle, uuids, opts.scanMode)
  }

  stopScan() {
    binding.centralStopScan(this._handle)
    this._peripherals.clear()
  }

  connect(peripheral) {
    binding.centralConnect(this._handle, peripheral.id)
    this._connected.set(peripheral.id, peripheral)
  }

  disconnect(peripheral) {
    if (peripheral._connectHandle) {
      binding.centralDisconnect(this._handle, peripheral._connectHandle)
    }

    this._connected.delete(peripheral.id)
  }

  destroy() {
    for (const peripheral of this._connected.values()) {
      peripheral.destroy()
    }
    this._connected.clear()
    binding.centralDestroy(this._handle)
  }

  [Symbol.for('bare.inspect')]() {
    return {
      __proto__: { constructor: Central },
      state: this._state
    }
  }

  _onstatechange(state) {
    this._state = STATES[state] || 'off'
    this.emit('stateChange', this._state)
  }

  _ondiscover(id, name, rssi, scanRecordData) {
    const device = new Device({ address: id, name })
    const scanRecord = new ScanRecord(scanRecordData)
    const scanResult = new ScanResult({ device, rssi, scanRecord })

    let peripheral = this._peripherals.get(id)

    if (peripheral) {
      peripheral._scanResult = scanResult
    } else {
      peripheral = new Peripheral({ scanResult })
      this._peripherals.set(id, peripheral)
    }

    this.emit('discover', peripheral)
  }

  _onconnect(gattHandle, id) {
    const peripheral = this._connected.get(id)

    if (peripheral) peripheral._attach(gattHandle)

    this.emit('connect', peripheral)
  }

  _ondisconnect(id, error) {
    const peripheral = this._connected.get(id) || null

    if (peripheral) peripheral._ondisconnect(error || null)

    this._connected.delete(id)

    this.emit('disconnect', peripheral, error || null)
  }

  _onconnectfail(id, error) {
    this.emit('connectFail', id, error)
  }

  _onscanfail(errorCode) {
    this.emit('scanFail', errorCode)
  }
}

exports.STATE_OFF = binding.STATE_OFF
exports.STATE_TURNING_ON = binding.STATE_TURNING_ON
exports.STATE_ON = binding.STATE_ON
exports.STATE_TURNING_OFF = binding.STATE_TURNING_OFF

exports.SCAN_MODE_OPPORTUNISTIC = binding.SCAN_MODE_OPPORTUNISTIC
exports.SCAN_MODE_LOW_POWER = binding.SCAN_MODE_LOW_POWER
exports.SCAN_MODE_BALANCED = binding.SCAN_MODE_BALANCED
exports.SCAN_MODE_LOW_LATENCY = binding.SCAN_MODE_LOW_LATENCY
