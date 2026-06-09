import Device from './device'
import ScanRecord from './scan-record'

export default class ScanResult {
  readonly device: Device
  readonly rssi: number
  readonly scanRecord: ScanRecord | null
}
