import Device from './device'
import ScanRecord from './scan-record'

export default class ScanResult {
  /** The `Device` the advertisement came from, mirroring Android's `ScanResult.getDevice()`. */
  readonly device: Device
  /** The signal strength of the most recent advertisement, equal to `scanResult.rssi`. */
  readonly rssi: number
  /** The `ScanRecord` parsed from the advertisement, or `null` when the advertisement carried no scan record. Mirrors `ScanResult.getScanRecord()`. */
  readonly scanRecord: ScanRecord | null
}
