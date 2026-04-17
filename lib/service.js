module.exports = class Service {
  constructor(uuid, characteristics, opts = {}) {
    this._uuid = uuid
    this._characteristics = characteristics || []
    this._primary = opts.primary !== false
    this._handle = null
  }

  get uuid() {
    return this._uuid
  }

  get characteristics() {
    return this._characteristics
  }

  get primary() {
    return this._primary
  }

  [Symbol.for('bare.inspect')]() {
    return {
      __proto__: { constructor: Service },
      uuid: this._uuid
    }
  }
}
