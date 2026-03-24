module.exports = exports = class Characteristic {
  constructor(uuid, opts = {}) {
    this._uuid = uuid
    this._properties = 0
    this._permissions = opts.permissions === undefined ? null : opts.permissions
    this._handle = null
    this._value = opts.value || null

    if (opts.read) this._properties |= Characteristic.PROPERTY_READ
    if (opts.write) this._properties |= Characteristic.PROPERTY_WRITE
    if (opts.writeWithoutResponse) {
      this._properties |= Characteristic.PROPERTY_WRITE_WITHOUT_RESPONSE
    }
    if (opts.notify) this._properties |= Characteristic.PROPERTY_NOTIFY
    if (opts.indicate) this._properties |= Characteristic.PROPERTY_INDICATE
  }

  /** @returns {string} */
  get uuid() {
    return this._uuid
  }

  /** @returns {number} Bitmask of characteristic properties */
  get properties() {
    return this._properties
  }

  /** @returns {number|null} Bitmask of characteristic permissions if set explicitly */
  get permissions() {
    return this._permissions
  }

  /** @returns {Uint8Array|null} The current value if set */
  get value() {
    return this._value
  }

  set value(val) {
    this._value = val
  }

  [Symbol.for('bare.inspect')]() {
    return {
      __proto__: { constructor: Characteristic },
      uuid: this._uuid
    }
  }
}
