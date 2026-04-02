export default class Characteristic {
  constructor(uuid: string, opts?: CharacteristicOptions)

  readonly uuid: string
  readonly properties: number
  readonly permissions: number | null
  value: Uint8Array | null

  static readonly PROPERTY_READ: number
  static readonly PROPERTY_WRITE_WITHOUT_RESPONSE: number
  static readonly PROPERTY_WRITE: number
  static readonly PROPERTY_NOTIFY: number
  static readonly PROPERTY_INDICATE: number
}

export interface CharacteristicOptions {
  read?: boolean
  write?: boolean
  writeWithoutResponse?: boolean
  notify?: boolean
  indicate?: boolean
  permissions?: number
  value?: Uint8Array | null
}
