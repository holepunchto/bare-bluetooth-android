export default class Characteristic {
  /**
   * @param uuid - The characteristic's UUID.
   * @param opts - Options selecting the characteristic `properties`, `permissions`, and initial
   * `value`.
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
  /** Whether to set the `PROPERTY_READ` flag in the characteristic's `properties`. */
  read?: boolean
  /** Whether to set the `PROPERTY_WRITE` flag in the characteristic's `properties`. */
  write?: boolean
  writeWithoutResponse?: boolean
  notify?: boolean
  indicate?: boolean
  permissions?: number
  value?: Uint8Array | null
}
