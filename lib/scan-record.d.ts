export type ServiceData = {
  [uuid: string]: Uint8Array
}

export default class ScanRecord {
  /** The advertised service data, or `null` when the advertisement carried no scan record or no service data. */
  readonly serviceData: ServiceData | null
}
