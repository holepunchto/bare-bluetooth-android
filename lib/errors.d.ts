export default class BluetoothError extends Error {
  /**
   * @param msg - A human-readable error message.
   * @param fn - The function to omit from the captured stack trace (default `BluetoothError`).
   * @param code - The error code; defaults to `fn.name` (e.g. `SCAN_FAILED`).
   */
  constructor(msg: string, fn?: Function, code?: string)

  readonly code: string
  /** The advertised name of the peripheral, or `null` if unavailable. Equal to `scanResult.device.name`. */
  readonly name: 'BluetoothError'
  /** The unique identifier of the peripheral, equal to `scanResult.device.address`. */
  id?: string

  static ADVERTISE_FAILED(msg: string): BluetoothError
  static SCAN_FAILED(msg: string): BluetoothError
  /**
   * @param msg - A human-readable error message.
   * @param id - The id of the peripheral the connection attempt targeted.
   */
  static CONNECTION_FAILED(msg: string, id: string): BluetoothError
  /**
   * @param msg - A human-readable error message.
   * @param id - The id of the peripheral that disconnected.
   */
  static DISCONNECT(msg: string, id: string): BluetoothError
  static DISCOVER_FAILED(msg: string): BluetoothError
  static READ_FAILED(msg: string): BluetoothError
  static WRITE_FAILED(msg: string): BluetoothError
  static NOTIFY_FAILED(msg: string): BluetoothError
  static NOTIFY_STATE_FAILED(msg: string): BluetoothError
  static CHANNEL_FAILED(msg: string): BluetoothError
  static SERVICE_ADD_FAILED(msg: string): BluetoothError
  static CHANNEL_PUBLISH_FAILED(msg: string): BluetoothError
  static MTU_CHANGE_FAILED(msg: string): BluetoothError
}
