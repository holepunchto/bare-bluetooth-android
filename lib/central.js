const EventEmitter = require('bare-events')
const binding = require('../binding')
const Peripheral = require('./peripheral')

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

  startScan(serviceUUIDs) {
    const uuids = serviceUUIDs ? serviceUUIDs.map((s) => binding.createUUID(s)) : null
    binding.centralStartScan(this._handle, uuids)
  }

  stopScan() {
    binding.centralStopScan(this._handle)
    this._peripherals.clear()
  }

  connect(peripheral) {
    binding.centralConnect(this._handle, peripheral.handle)
  }

  disconnect(peripheral) {
    binding.centralDisconnect(this._handle, peripheral._connectHandle)
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

  _ondiscover(handle, id, name, rssi) {
    let peripheral = this._peripherals.get(id)

    if (peripheral) {
      peripheral.handle = handle
      peripheral.rssi = rssi
      if (name) peripheral.name = name
    } else {
      peripheral = { handle, id, name, rssi }
      this._peripherals.set(id, peripheral)
    }

    this.emit('discover', peripheral)
  }

  _onconnect(handle, id) {
    const discovered = this._peripherals.get(id) || null
    const peripheral = new Peripheral(handle, {
      id,
      name: discovered ? discovered.name : null,
      connectHandle: handle
    })
    this._connected.set(id, peripheral)

    this.emit('connect', peripheral)
  }

  _ondisconnect(id, error) {
    const peripheral = this._connected.get(id) || null

    if (peripheral) {
      peripheral._ondisconnect(error || null)
      peripheral.destroy()
    }

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
