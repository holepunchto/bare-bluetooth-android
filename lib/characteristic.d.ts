/** Create a new GATT characteristic definition. */
export default class Characteristic {
  /**
   * @param uuid - The characteristic's UUID.
   * @param opts - Options selecting the characteristic `properties`, `permissions`, and initial `value`.
   */
  constructor(uuid: string, opts?: CharacteristicOptions)

  /** The UUID of the characteristic. */
  readonly uuid: string
  /** The property flags of the characteristic. */
  readonly properties: number
  /** The permission flags of the characteristic, or `null` if inferred. */
  readonly permissions: number | null
  /** The current value of the characteristic, or `null`. */
  value: Uint8Array | null

  static readonly PROPERTY_READ: number
  static readonly PROPERTY_WRITE_WITHOUT_RESPONSE: number
  static readonly PROPERTY_WRITE: number
  static readonly PROPERTY_NOTIFY: number
  /** Characteristic property constants. */
  static readonly PROPERTY_INDICATE: number
}

export interface CharacteristicOptions {
  /**
   * Read the value of `characteristic`. The result is emitted via the `'read'` event.
   * @param characteristic - The characteristic to read.
   */
  read?: boolean
  /**
   * Write `data` to `characteristic`. If `withResponse` is `true` (the default), a write confirmation is requested.
   * @param characteristic - The characteristic to write to.
   * @param data - The bytes to write.
   * @param withResponse - Whether a write confirmation is requested (default `true`).
   */
  write?: boolean
  writeWithoutResponse?: boolean
  notify?: boolean
  indicate?: boolean
  permissions?: number
  value?: Uint8Array | null
}
