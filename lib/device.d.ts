export default class Device {
  /** The hardware address of the device, mirroring `BluetoothDevice.getAddress()`. */
  readonly address: string
  /** The advertised name of the peripheral, or `null` if unavailable. Equal to `scanResult.device.name`. */
  readonly name: string | null
}
