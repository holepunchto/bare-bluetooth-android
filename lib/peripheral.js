const EventEmitter = require('bare-events')
const binding = require('../binding')
const Service = require('./service')
const Characteristic = require('./characteristic')
const L2CAPChannel = require('./channel')
const errors = require('./errors')

module.exports = exports = class Peripheral extends EventEmitter {
  constructor(opts) {
    super()

    this._scanResult = opts.scanResult
    this._destroyed = false
    this._services = new Map()
    this._characteristics = new Map()
    this._characteristicsByHandle = new Map()
    this._gattHandle = null
    this._handle = null
    // Android allows one GATT operation in flight per connection; a second
    // initiate is rejected or clobbers the pending one. Ops run serially,
    // advancing on the op's completion event, failure, or timeout.
    this._gattQueue = []
    this._gattInFlight = false
    this._gattCancel = null
  }

  _enqueueGatt(name, run, event, failCode) {
    this._gattQueue.push({ name, run, event, failCode })
    this._drainGatt()
  }

  // Peripherals are reused across reconnects: a stale finish() firing after
  // a redial would double-issue the new in-flight op, so cancel it fully.
  _resetGattQueue() {
    if (this._gattCancel) this._gattCancel()
    this._gattQueue = []
    this._gattInFlight = false
  }

  _drainGatt() {
    if (this._gattInFlight || this._gattQueue.length === 0) return
    if (this._destroyed || !this._handle) {
      this._gattQueue = []
      return
    }
    const op = this._gattQueue[0]
    this._gattInFlight = true

    const cleanup = () => {
      if (op._timer) {
        clearTimeout(op._timer)
        op._timer = null
      }
      this.removeListener(op.event, onEvent)
      this.removeListener('error', onError)
      if (this._gattCancel === cancel) this._gattCancel = null
    }
    // stop the op without advancing the queue — the caller is resetting it
    const cancel = () => {
      if (op._done) return
      op._done = true
      cleanup()
    }
    const finish = () => {
      if (op._done) return
      op._done = true
      cleanup()
      if (this._gattQueue[0] === op) this._gattQueue.shift()
      this._gattInFlight = false
      this._drainGatt()
    }
    // fail the op with its own error code, emitting before the next op is
    // drained so the emit cannot complete a same-typed successor
    const fail = (message) => {
      if (op._done) return
      op._done = true
      cleanup()
      this._gattQueue.shift()
      this._gattInFlight = false
      this.emit('error', errors[op.failCode](op.name + ' ' + message))
      this._drainGatt()
    }
    const onEvent = () => finish()
    // only this op's own failure code completes it — an unrelated error must
    // not advance the queue while the radio is still busy with this op
    const onError = (err) => {
      if (err && err.code === op.failCode) finish()
    }

    this._gattCancel = cancel

    let ok
    try {
      ok = op.run()
    } catch {
      ok = false
    }
    // undefined (void binding) means initiated; only false is a rejection.
    // Fail fast: Android itself retries a transient busy before returning
    // false, so a rejection is a real failure the caller must see.
    if (ok === false) return fail('rejected by the stack')
    this.once(op.event, onEvent)
    this.on('error', onError)
    // a lost completion must not wedge the queue: fail the op so the caller
    // sees it — except requestMtu, which is advisory and must not kill a
    // healthy link over a missing mtuChanged
    const onExpire = op.name === 'requestMtu' ? finish : () => fail('completion timed out')
    op._timer = setTimeout(onExpire, 10000)
    if (op._timer.unref) op._timer.unref()
  }

  get scanResult() {
    return this._scanResult
  }

  get id() {
    return this._scanResult.device.address
  }

  get name() {
    return this._scanResult.device.name
  }

  get rssi() {
    return this._scanResult.rssi
  }

  get serviceData() {
    return this._scanResult.scanRecord ? this._scanResult.scanRecord.serviceData : null
  }

  _attach(gattHandle) {
    this._gattHandle = gattHandle
    this._handle = binding.peripheralInit(
      gattHandle,
      this,
      this._onservicesdiscover,
      this._oncharacteristicsdiscover,
      this._onread,
      this._onwrite,
      this._onnotify,
      this._onnotifystate,
      this._onchannelopen,
      this._onmtuchanged
    )
  }

  discoverServices() {
    this._enqueueGatt(
      'discoverServices',
      () => binding.peripheralDiscoverServices(this._handle),
      'servicesDiscover',
      'DISCOVER_FAILED'
    )
  }

  discoverCharacteristics(service) {
    this._enqueueGatt(
      'discoverCharacteristics',
      () => binding.peripheralDiscoverCharacteristics(this._handle, service._handle),
      'characteristicsDiscover',
      'DISCOVER_FAILED'
    )
  }

  read(characteristic) {
    this._enqueueGatt(
      'read',
      () => binding.peripheralRead(this._handle, characteristic._handle),
      'read',
      'READ_FAILED'
    )
  }

  write(characteristic, data, withResponse) {
    if (withResponse === undefined) withResponse = true
    this._enqueueGatt(
      'write',
      () => binding.peripheralWrite(this._handle, characteristic._handle, data, withResponse),
      'write',
      'WRITE_FAILED'
    )
  }

  subscribe(characteristic) {
    this._enqueueGatt(
      'subscribe',
      () => binding.peripheralSubscribe(this._handle, characteristic._handle),
      'notifyState',
      'NOTIFY_STATE_FAILED'
    )
  }

  unsubscribe(characteristic) {
    this._enqueueGatt(
      'unsubscribe',
      () => binding.peripheralUnsubscribe(this._handle, characteristic._handle),
      'notifyState',
      'NOTIFY_STATE_FAILED'
    )
  }

  openL2CAPChannel(psm) {
    binding.peripheralOpenL2CAPChannel(this._handle, psm)
  }

  requestMtu(mtu) {
    this._enqueueGatt(
      'requestMtu',
      () => binding.peripheralRequestMtu(this._handle, mtu),
      'mtuChanged',
      'MTU_CHANGE_FAILED'
    )
  }

  destroy() {
    if (this._destroyed) return
    this._destroyed = true

    this._resetGattQueue()
    if (this._handle) {
      binding.peripheralDestroy(this._handle)
      this._handle = null
      this._gattHandle = null
    }
  }

  [Symbol.for('bare.inspect')]() {
    return {
      __proto__: { constructor: Peripheral },
      id: this.id,
      name: this.name,
      serviceData: this.serviceData
    }
  }

  _onservicesdiscover(count, error) {
    if (error) {
      this.emit('error', errors.DISCOVER_FAILED(error))
      return
    }

    const services = []

    for (let i = 0; i < count; i++) {
      const handle = binding.peripheralServiceAtIndex(this._handle, i)
      const serviceKey = binding.serviceKey(handle)
      const uuid = binding.serviceUuid(handle)
      let service = this._services.get(serviceKey)

      if (!service) {
        service = new Service(uuid)
        service._handle = handle
        service._key = serviceKey
        this._services.set(serviceKey, service)
      } else {
        service._handle = handle
      }

      services.push(service)
    }

    this.emit('servicesDiscover', services)
  }

  _oncharacteristicsdiscover(serviceHandle, count, error) {
    const serviceKey = serviceHandle ? binding.serviceKey(serviceHandle) : null
    const service = serviceKey === null ? null : this._services.get(serviceKey) || null

    if (error) {
      this.emit('error', errors.DISCOVER_FAILED(error))
      return
    }

    const characteristics = []

    for (let i = 0; i < count; i++) {
      const handle = binding.serviceCharacteristicAtIndex(serviceHandle, i)
      const handleKey = binding.characteristicKey(handle)
      const characteristicKey = serviceKey + ':' + handleKey
      const uuid = binding.characteristicUuid(handle)
      const properties = binding.characteristicProperties(handle)
      let characteristic = this._characteristics.get(characteristicKey)

      if (!characteristic) {
        characteristic = new Characteristic(uuid)
        characteristic._handle = handle
        characteristic._properties = properties
        characteristic._key = characteristicKey
        characteristic._serviceKey = serviceKey
        characteristic._handleKey = handleKey
        this._characteristics.set(characteristicKey, characteristic)
      } else {
        characteristic._handle = handle
        characteristic._properties = properties
      }

      this._characteristicsByHandle.set(handleKey, characteristic)

      characteristics.push(characteristic)
    }

    if (service) {
      service._characteristics = characteristics
    }

    this.emit('characteristicsDiscover', service, characteristics)
  }

  _onread(charHandle, uuid, data, error) {
    if (error) {
      this.emit('error', errors.READ_FAILED(error))
      return
    }

    const characteristic = charHandle
      ? this._characteristicsByHandle.get(binding.characteristicKey(charHandle)) || null
      : null

    this.emit('read', characteristic, data)
  }

  _onwrite(charHandle, uuid, error) {
    if (error) {
      this.emit('error', errors.WRITE_FAILED(error))
      return
    }

    const characteristic = charHandle
      ? this._characteristicsByHandle.get(binding.characteristicKey(charHandle)) || null
      : null

    this.emit('write', characteristic)
  }

  _onnotify(uuid, data, error) {
    if (error) {
      this.emit('error', errors.NOTIFY_FAILED(error))
      return
    }

    const characteristic = this._characteristicsByHandle.get(uuid) || null
    this.emit('notify', characteristic, data)
  }

  _onnotifystate(charHandle, uuid, isNotifying, error) {
    if (error) {
      this.emit('error', errors.NOTIFY_STATE_FAILED(error))
      return
    }

    const characteristic = charHandle
      ? this._characteristicsByHandle.get(binding.characteristicKey(charHandle)) || null
      : null

    this.emit('notifyState', characteristic, isNotifying)
  }

  _ondisconnect(error) {
    this._resetGattQueue()
    if (this._handle) {
      binding.peripheralDestroy(this._handle)
      this._handle = null
    }

    this._services.clear()
    this._characteristics.clear()
    this._characteristicsByHandle.clear()

    if (error) {
      this.emit('error', errors.DISCONNECT(error))
      return
    }

    this.emit('disconnect')
  }

  _onchannelopen(channelHandle, error) {
    if (error || !channelHandle) {
      this.emit('error', errors.CHANNEL_FAILED(error || 'Channel open failed'))
      return
    }

    const channel = new L2CAPChannel(channelHandle)
    this.emit('channelOpen', channel)
  }

  _onmtuchanged(mtu, error) {
    if (error) {
      this.emit('error', errors.MTU_CHANGE_FAILED(error))
      return
    }

    this.emit('mtuChanged', mtu)
  }
}

exports.PROPERTY_READ = binding.PROPERTY_READ
exports.PROPERTY_WRITE_WITHOUT_RESPONSE = binding.PROPERTY_WRITE_WITHOUT_RESPONSE
exports.PROPERTY_WRITE = binding.PROPERTY_WRITE
exports.PROPERTY_NOTIFY = binding.PROPERTY_NOTIFY
exports.PROPERTY_INDICATE = binding.PROPERTY_INDICATE
