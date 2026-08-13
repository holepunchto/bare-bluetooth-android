export default class BluetoothError extends Error {
  /**
   * @param msg - A human-readable error message.
   * @param fn - The function to omit from the captured stack trace (default `BluetoothError`).
   * @param code - The error code; defaults to `fn.name` (for example `SCAN_FAILED`).
   */
  constructor(msg: string, fn?: Function, code?: string)

  readonly code: string
  /** The name of the error, always `'BluetoothError'`. */
  readonly name: 'BluetoothError'
  /**
   * The id of the peripheral, as passed to `BluetoothError.CONNECTION_FAILED()` or
   * `BluetoothError.DISCONNECT()`. Left unset by every other factory.
   */
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
