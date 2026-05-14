const EventEmitter = require('bare-events')
const binding = require('../binding')
const Service = require('./service')
const Characteristic = require('./characteristic')
const L2CAPChannel = require('./channel')

module.exports = exports = class Peripheral extends EventEmitter {
  constructor(opts) {
    super()

    if (!opts || !opts.scanResult) {
      throw new Error('scanResult is required')
    }

    this._scanResult = opts.scanResult
    this._destroyed = false
    this._services = new Map()
    this._characteristics = new Map()
    this._characteristicsByHandle = new Map()
    this._handle = null
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
    const record = this._scanResult.scanRecord
    return record ? record.serviceData : null
  }

  get _deviceHandle() {
    return this._scanResult.device._handle
  }

  _attach(peripheralHandle) {
    this._handle = binding.peripheralInit(
      peripheralHandle,
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
    return binding.peripheralDiscoverServices(this._handle)
  }

  discoverCharacteristics(service) {
    binding.peripheralDiscoverCharacteristics(this._handle, service._handle)
  }

  read(characteristic) {
    return binding.peripheralRead(this._handle, characteristic._handle)
  }

  write(characteristic, data, withResponse) {
    if (withResponse === undefined) withResponse = true
    return binding.peripheralWrite(this._handle, characteristic._handle, data, withResponse)
  }

  subscribe(characteristic) {
    return binding.peripheralSubscribe(this._handle, characteristic._handle)
  }

  unsubscribe(characteristic) {
    return binding.peripheralUnsubscribe(this._handle, characteristic._handle)
  }

  openL2CAPChannel(psm) {
    binding.peripheralOpenL2CAPChannel(this._handle, psm)
  }

  requestMtu(mtu) {
    return binding.peripheralRequestMtu(this._handle, mtu)
  }

  destroy() {
    if (this._destroyed) return
    this._destroyed = true
    if (this._handle) {
      binding.peripheralDestroy(this._handle)
      this._handle = null
    }
  }

  _ondisconnect(error) {
    if (this._handle) {
      binding.peripheralDestroy(this._handle)
      this._handle = null
    }

    this._services.clear()
    this._characteristics.clear()
    this._characteristicsByHandle.clear()

    this.emit('disconnect', error)
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
      this.emit('servicesDiscover', null, error)
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

    this.emit('servicesDiscover', services, null)
  }

  _oncharacteristicsdiscover(serviceHandle, count, error) {
    const serviceKey = binding.serviceKey(serviceHandle)
    const service = this._services.get(serviceKey) || null

    if (error) {
      this.emit('characteristicsDiscover', service, null, error)
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

    this.emit('characteristicsDiscover', service, characteristics, null)
  }

  _onread(charHandle, uuid, data, error) {
    const characteristic =
      this._characteristicsByHandle.get(binding.characteristicKey(charHandle)) || null

    this.emit('read', characteristic, data, error)
  }

  _onwrite(charHandle, uuid, error) {
    const characteristic =
      this._characteristicsByHandle.get(binding.characteristicKey(charHandle)) || null

    this.emit('write', characteristic, error)
  }

  _onnotify(uuid, data, error) {
    const characteristic = this._characteristicsByHandle.get(uuid) || null
    this.emit('notify', characteristic, data, error)
  }

  _onnotifystate(charHandle, uuid, isNotifying, error) {
    const characteristic =
      this._characteristicsByHandle.get(binding.characteristicKey(charHandle)) || null

    this.emit('notifyState', characteristic, isNotifying, error)
  }

  _onchannelopen(channelHandle, error) {
    if (error || !channelHandle) {
      this.emit('channelOpen', null, error)
      return
    }

    const channel = new L2CAPChannel(channelHandle)
    this.emit('channelOpen', channel, null)
  }

  _onmtuchanged(mtu, error) {
    this.emit('mtuChanged', mtu, error)
  }
}

exports.PROPERTY_READ = binding.PROPERTY_READ
exports.PROPERTY_WRITE_WITHOUT_RESPONSE = binding.PROPERTY_WRITE_WITHOUT_RESPONSE
exports.PROPERTY_WRITE = binding.PROPERTY_WRITE
exports.PROPERTY_NOTIFY = binding.PROPERTY_NOTIFY
exports.PROPERTY_INDICATE = binding.PROPERTY_INDICATE
