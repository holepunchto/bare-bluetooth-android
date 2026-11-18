export type ServiceData = {
  [uuid: string]: Uint8Array
}

export default class ScanRecord {
  /** The advertised service data, or `null` when the scan record carried no service data. */
  readonly serviceData: ServiceData | null
  /** The advertised service UUIDs, empty when the scan record carried none. */
  readonly serviceUUIDs: string[]
}
