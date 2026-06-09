export type ServiceData = {
  [uuid: string]: Uint8Array
}

export default class ScanRecord {
  readonly serviceData: ServiceData | null
}
