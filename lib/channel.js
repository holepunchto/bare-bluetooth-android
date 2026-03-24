const EventEmitter = require('bare-events')
const binding = require('../binding')

module.exports = exports = class L2CAPChannel extends EventEmitter {
  constructor(channelHandle, opts = {}) {
    super()

    this._channelHandle = channelHandle
    this._psm = opts.psm || 0
    this._destroyed = false

    this._handle = binding.l2capInit(
      channelHandle,
      this,
      this._ondata,
      this._ondrain,
      this._onend,
      this._onerror,
      this._onclose,
      this._onopen
    )
  }

  /** @returns {number} The L2CAP PSM for this channel. */
  get psm() {
    return this._psm
  }

  /**
   * @returns {string|null}
   */
  get peer() {
    return binding.l2capPeer(this._handle)
  }

  open() {
    binding.l2capOpen(this._handle)
  }

  /**
   * @param {Uint8Array} data
   * @returns {number} Number of bytes queued (0 if channel is destroyed)
   */
  write(data) {
    if (this._destroyed) return 0
    return binding.l2capWrite(this._handle, data)
  }

  destroy() {
    if (this._destroyed) return
    this._destroyed = true
    binding.l2capEnd(this._handle)
  }

  [Symbol.for('bare.inspect')]() {
    return {
      __proto__: { constructor: L2CAPChannel },
      destroyed: this._destroyed
    }
  }

  _ondata(data) {
    this.emit('data', data)
  }

  _ondrain() {
    this.emit('drain')
  }

  _onend() {
    this.emit('end')
  }

  _onerror(message) {
    this.emit('error', new Error(message))
  }

  _onclose() {
    this.emit('close')
  }

  _onopen() {
    this.emit('open')
  }
}
