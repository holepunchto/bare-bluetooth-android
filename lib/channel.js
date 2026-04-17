const { Duplex } = require('bare-stream')

const binding = require('../binding')

module.exports = exports = class L2CAPChannel extends Duplex {
  constructor(channelHandle) {
    super({
      allowHalfOpen: false
    })

    this._channelHandle = channelHandle

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

  get psm() {
    return binding.l2capPsm(this._handle)
  }

  get peer() {
    return binding.l2capPeer(this._handle)
  }

  _open(cb) {
    binding.l2capOpen(this._handle)
    cb()
  }

  _write(chunk, _encoding, cb) {
    const bytesWritten = binding.l2capWrite(this._handle, chunk)
    if (bytesWritten === 0) {
      cb(new Error('Channel not ready or destroyed'))
    } else {
      cb(null)
    }
  }

  _destroy(err, cb) {
    binding.l2capEnd(this._handle)
    cb(err)
  }

  [Symbol.for('bare.inspect')]() {
    return {
      __proto__: { constructor: L2CAPChannel },
      destroyed: this.destroyed
    }
  }

  _ondata(data) {
    this.push(data)
  }

  _ondrain() {
    this.emit('drain')
  }

  _onend() {
    this.push(null)
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
