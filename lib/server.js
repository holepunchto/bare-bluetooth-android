const EventEmitter = require('bare-events')
const binding = require('../binding')
const L2CAPChannel = require('./channel')
const constants = require('./constants')

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

  /**
   * The current Bluetooth adapter state.
   *
   * Android-specific values: `'off'`, `'turningOn'`, `'on'`, `'turningOff'`.
   *
   * @returns {string}
   */
  get state() {
    return this._state
  }

  /**
   * Add a GATT service with its characteristics to the server.
   *
   * On Android, characteristics are added to the service via
   * `BluetoothGattService.addCharacteristic()` — there is no separate
   * `serviceSetCharacteristics` step.
   *
   * @param {Service} service
   */
  addService(service) {
    const charHandles = []

    for (const char of service.characteristics) {
      let permissions = char.permissions

      if (permissions === null) {
        permissions = 0

        if (char.properties & constants.PROPERTY_READ) {
          permissions |= constants.PERMISSION_READABLE
        }

        if (char.properties & (constants.PROPERTY_WRITE | constants.PROPERTY_WRITE_WITHOUT_RESPONSE)) {
          permissions |= constants.PERMISSION_WRITEABLE
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

  /**
   * Start BLE advertising.
   *
   * @param {object} [opts]
   * @param {string} [opts.name]
   * @param {string[]} [opts.serviceUUIDs]
   */
  startAdvertising(opts = {}) {
    const uuids = opts.serviceUUIDs ? opts.serviceUUIDs.map((s) => binding.createUUID(s)) : null
    binding.serverStartAdvertising(this._handle, opts.name || null, uuids)
  }

  stopAdvertising() {
    binding.serverStopAdvertising(this._handle)
  }

  /**
   * Respond to a read or write request from a central.
   *
   * On Android, `sendResponse` requires the requestId and offset from the
   * original request. These are stored on the request object.
   *
   * @param {object} request
   * @param {number} result - ATT result code.
   * @param {Uint8Array|null} [data]
   */
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

  /**
   * Push an updated value to all centrals subscribed to a characteristic.
   *
   * On Android this iterates all subscribed devices and calls
   * `notifyCharacteristicChanged` for each one. Always returns `true` — there
   * is no backpressure mechanism on Android (no `readyToUpdate` event).
   *
   * @param {Characteristic} characteristic
   * @param {Uint8Array} data
   * @returns {boolean}
   */
  updateValue(characteristic, data) {
    return binding.serverUpdateValue(this._handle, characteristic._handle, data)
  }

  /**
   * @param {object} [opts]
   * @param {boolean} [opts.encrypted=false]
   */
  publishChannel(opts = {}) {
    binding.serverPublishChannel(this._handle, opts.encrypted || false)
  }

  /**
   * @param {number} psm
   */
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
    this._state = constants.STATES[state] || 'off'
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

  // TODO: Android fires one write request per callback while Apple can batch
  // multiple writes into a single callback. Both emit an array for API
  // compatibility but Android always emits a 1-element array.
  _onwriterequest(requestHandle, requestId, characteristicUuid, offset, data, responseNeeded) {
    this.emit('writeRequest', [{
      handle: requestHandle,
      requestId,
      characteristicUuid,
      data,
      offset,
      responseNeeded
    }])
  }

  /**
   * On Android the first argument is a device MAC address string. On Apple it
   * is a native CBCentral handle. Consumers writing cross-platform server code
   * should be aware of this difference.
   */
  _onsubscribe(deviceAddress, characteristicUuid) {
    this.emit('subscribe', deviceAddress, characteristicUuid)
  }

  /**
   * See `_onsubscribe` for platform differences in the first argument.
   */
  _onunsubscribe(deviceAddress, characteristicUuid) {
    this.emit('unsubscribe', deviceAddress, characteristicUuid)
  }

  /**
   * Android-specific. Fired when `startAdvertising()` fails.
   *
   * Error codes:
   * - 1: Data too large
   * - 2: Too many advertisers
   * - 3: Already started
   * - 4: Internal error
   * - 5: Feature unsupported
   */
  _onadvertiseerror(errorCode, error) {
    this.emit('advertiseError', errorCode, error)
  }

  _onchannelpublish(psm, error) {
    this.emit('channelPublish', psm, error)
  }

  _onchannelopen(channelHandle, error, psm) {
    if (error || !channelHandle) {
      this.emit('channelOpen', null, error)
      return
    }

    const channel = new L2CAPChannel(channelHandle, { psm })
    channel.open()
    this.emit('channelOpen', channel, null)
  }
}
