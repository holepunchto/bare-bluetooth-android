const EventEmitter = require('bare-events')
const binding = require('../binding')
const L2CAPChannel = require('./channel')

const STATES = {
  [binding.STATE_OFF]: 'off',
  [binding.STATE_TURNING_ON]: 'turningOn',
  [binding.STATE_ON]: 'on',
  [binding.STATE_TURNING_OFF]: 'turningOff'
}

module.exports = exports = class Server extends EventEmitter {
  constructor() {
    super()

    this._state = 'off'

    this._handle = binding.serverInit(
      this,
      this._onstatechange,
      this._onaddservice,
      this._onreadrequest,
      this._onwriterequest,
      this._onsubscribe,
      this._onunsubscribe,
      this._onadvertiseerror,
      this._onchannelpublish,
      this._onchannelopen
    )
  }

  get state() {
    return this._state
  }

  addService(service) {
    const charHandles = []

    for (const char of service.characteristics) {
      let permissions = char.permissions

      if (permissions === null) {
        permissions = 0

        if (char.properties & exports.PROPERTY_READ) {
          permissions |= exports.PERMISSION_READABLE
        }

        if (char.properties & (exports.PROPERTY_WRITE | exports.PROPERTY_WRITE_WITHOUT_RESPONSE)) {
          permissions |= exports.PERMISSION_WRITEABLE
        }
      }

      const uuid = binding.createUUID(char.uuid)
      char._handle = binding.createMutableCharacteristic(
        uuid,
        char.properties,
        permissions,
        char.value || null
      )
      charHandles.push(char._handle)
    }

    const serviceUuid = binding.createUUID(service.uuid)
    const serviceHandle = binding.createMutableService(serviceUuid, service.primary)
    binding.serviceSetCharacteristics(serviceHandle, charHandles)
    binding.serverAddService(this._handle, serviceHandle)
  }

  startAdvertising(opts = {}) {
    const uuids = opts.serviceUUIDs ? opts.serviceUUIDs.map((s) => binding.createUUID(s)) : null
    binding.serverStartAdvertising(this._handle, opts.name || null, uuids)
  }

  stopAdvertising() {
    binding.serverStopAdvertising(this._handle)
  }

  respondToRequest(request, result, data) {
    binding.serverRespondToRequest(
      this._handle,
      request.handle,
      request.requestId,
      result,
      request.offset,
      data || null
    )
  }

  updateValue(characteristic, data) {
    return binding.serverUpdateValue(this._handle, characteristic._handle, data)
  }

  publishChannel(opts = {}) {
    binding.serverPublishChannel(this._handle, opts.encrypted || false)
  }

  unpublishChannel(psm) {
    binding.serverUnpublishChannel(this._handle, psm)
  }

  destroy() {
    binding.serverDestroy(this._handle)
  }

  [Symbol.for('bare.inspect')]() {
    return {
      __proto__: { constructor: Server },
      state: this._state
    }
  }

  _onstatechange(state) {
    this._state = STATES[state] || 'off'
    this.emit('stateChange', this._state)
  }

  _onaddservice(uuid, error) {
    this.emit('serviceAdd', uuid, error)
  }

  _onreadrequest(requestHandle, requestId, characteristicUuid, offset) {
    this.emit('readRequest', {
      handle: requestHandle,
      requestId,
      characteristicUuid,
      offset
    })
  }

  _onwriterequest(requestHandle, requestId, characteristicUuid, offset, data, responseNeeded) {
    this.emit('writeRequest', [
      {
        handle: requestHandle,
        requestId,
        characteristicUuid,
        data,
        offset,
        responseNeeded
      }
    ])
  }

  _onsubscribe(deviceAddress, characteristicUuid) {
    this.emit('subscribe', deviceAddress, characteristicUuid)
  }

  _onunsubscribe(deviceAddress, characteristicUuid) {
    this.emit('unsubscribe', deviceAddress, characteristicUuid)
  }

  _onadvertiseerror(errorCode, error) {
    this.emit('advertiseError', errorCode, error)
  }

  _onchannelpublish(psm, error) {
    this.emit('channelPublish', psm, error)
  }

  _onchannelopen(channelHandle, error) {
    if (error || !channelHandle) {
      this.emit('channelOpen', null, error)
      return
    }

    const channel = new L2CAPChannel(channelHandle)
    this.emit('channelOpen', channel, null)
  }
}

exports.STATE_OFF = binding.STATE_OFF
exports.STATE_TURNING_ON = binding.STATE_TURNING_ON
exports.STATE_ON = binding.STATE_ON
exports.STATE_TURNING_OFF = binding.STATE_TURNING_OFF

exports.PROPERTY_READ = binding.PROPERTY_READ
exports.PROPERTY_WRITE_WITHOUT_RESPONSE = binding.PROPERTY_WRITE_WITHOUT_RESPONSE
exports.PROPERTY_WRITE = binding.PROPERTY_WRITE
exports.PROPERTY_NOTIFY = binding.PROPERTY_NOTIFY
exports.PROPERTY_INDICATE = binding.PROPERTY_INDICATE

exports.PERMISSION_READABLE = binding.PERMISSION_READABLE
exports.PERMISSION_WRITEABLE = binding.PERMISSION_WRITEABLE
exports.PERMISSION_READ_ENCRYPTED = binding.PERMISSION_READ_ENCRYPTED
exports.PERMISSION_WRITE_ENCRYPTED = binding.PERMISSION_WRITE_ENCRYPTED

exports.ATT_SUCCESS = binding.ATT_SUCCESS
exports.ATT_INVALID_HANDLE = binding.ATT_INVALID_HANDLE
exports.ATT_READ_NOT_PERMITTED = binding.ATT_READ_NOT_PERMITTED
exports.ATT_WRITE_NOT_PERMITTED = binding.ATT_WRITE_NOT_PERMITTED
exports.ATT_INSUFFICIENT_RESOURCES = binding.ATT_INSUFFICIENT_RESOURCES
exports.ATT_UNLIKELY_ERROR = binding.ATT_UNLIKELY_ERROR
