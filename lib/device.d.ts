export default class Device {
  /** The hardware address of the device, mirroring `BluetoothDevice.getAddress()`. */
  readonly address: string
  /** The advertised name of the device, or `null` if unavailable. */
  readonly name: string | null
}
