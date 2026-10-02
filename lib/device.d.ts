export default class Device {
  /** The hardware address of the device, mirroring `BluetoothDevice.getAddress()`. */
  readonly address: string
  /** The advertised name of the device, or `null` if unavailable. */
  readonly name: string | null
  /**
   * One of the `Device.DEVICE_TYPE_*` constants, mirroring `BluetoothDevice.getType()`, or `null`
   * when the device came from a scan.
   */
  readonly type: number | null

  static readonly DEVICE_TYPE_UNKNOWN: number
  static readonly DEVICE_TYPE_CLASSIC: number
  static readonly DEVICE_TYPE_LE: number
  static readonly DEVICE_TYPE_DUAL: number
}
