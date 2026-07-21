import { EventEmitter, EventMap } from 'bare-events'
import Service from './service'
import Characteristic from './characteristic'
import L2CAPChannel from './channel'
import BluetoothError from './errors'

export type BluetoothState = 'off' | 'turningOn' | 'on' | 'turningOff'

export interface AdvertisingOptions {
  /** The advertised name of the peripheral, or `null` if unavailable. Equal to `scanResult.device.name`. */
  name?: string
  serviceUUIDs?: string[]
}

export interface ChannelOptions {
  encrypted?: boolean
}

export interface ReadRequest {
  handle: unknown
  requestId: number
  characteristicUuid: string
  offset: number
}

export interface WriteRequest {
  handle: unknown
  requestId: number
  characteristicUuid: string
  data: Uint8Array
  offset: number
  responseNeeded: boolean
}

export interface ServerEventMap extends EventMap {
  stateChange: [state: BluetoothState]
  serviceAdd: [uuid: string]
  channelPublish: [psm: number]
  channelOpen: [channel: L2CAPChannel]
  readRequest: [request: ReadRequest]
  writeRequest: [requests: WriteRequest[]]
  /**
   * Subscribe to notifications for `characteristic`.
   * @param characteristic - The characteristic to start receiving notifications for.
   */
  subscribe: [deviceAddress: string, characteristicUuid: string]
  /**
   * Unsubscribe from notifications for `characteristic`.
   * @param characteristic - The characteristic to stop receiving notifications for.
   */
  unsubscribe: [deviceAddress: string, characteristicUuid: string]
  connecting: [deviceAddress: string]
  connected: [deviceAddress: string]
  disconnecting: [deviceAddress: string]
  disconnected: [deviceAddress: string]
  notifySent: [deviceAddress: string, status: number]
  error: [error: BluetoothError]
}

/** Create a new BLE peripheral server for advertising services and handling client requests. */
declare class Server extends EventEmitter<ServerEventMap> {
  constructor()

  /** The current Bluetooth adapter state. One of `'off'`, `'turningOn'`, `'on'`, or `'turningOff'`. */
  readonly state: BluetoothState

  /**
   * @param service - The `Service` to register with the GATT server.
   */
  addService(service: Service): void
  /**
   * @param opts - Advertising options such as the local `name` and the `serviceUUIDs` to advertise.
   */
  startAdvertising(opts?: AdvertisingOptions): void
  /** Stop advertising. */
  stopAdvertising(): void
  /**
   * @param request - The read or write request to respond to.
   * @param result - The ATT result code; use the `Server.ATT_*` constants.
   * @param data - The value to return for a read request; omit for write responses.
   */
  respondToRequest(request: ReadRequest, result: number, data?: Uint8Array): void
  /**
   * @param characteristic - The characteristic whose value changed.
   * @param data - The new value to send to subscribed clients.
   * @returns Whether the notification was sent to subscribed clients successfully.
   */
  updateValue(characteristic: Characteristic, data: Uint8Array): boolean
  /**
   * @param opts - Options for the L2CAP channel to publish.
   */
  publishChannel(opts?: ChannelOptions): void
  /**
   * @param psm - The PSM of the channel to unpublish, as assigned when it was published.
   */
  unpublishChannel(psm: number): void
  /** Destroy the server, stopping advertising and unpublishing all L2CAP channels. */
  destroy(): void

  static readonly STATE_OFF: number
  static readonly STATE_TURNING_ON: number
  static readonly STATE_ON: number
  static readonly STATE_TURNING_OFF: number

  static readonly PROPERTY_READ: number
  static readonly PROPERTY_WRITE_WITHOUT_RESPONSE: number
  static readonly PROPERTY_WRITE: number
  static readonly PROPERTY_NOTIFY: number
  /** Characteristic property constants. */
  static readonly PROPERTY_INDICATE: number

  static readonly PERMISSION_READABLE: number
  static readonly PERMISSION_WRITEABLE: number
  static readonly PERMISSION_READ_ENCRYPTED: number
  /** Characteristic permission constants. */
  static readonly PERMISSION_WRITE_ENCRYPTED: number

  static readonly CONNECTION_STATE_DISCONNECTED: number
  static readonly CONNECTION_STATE_CONNECTING: number
  static readonly CONNECTION_STATE_CONNECTED: number
  static readonly CONNECTION_STATE_DISCONNECTING: number

  static readonly ATT_SUCCESS: number
  static readonly ATT_INVALID_HANDLE: number
  static readonly ATT_READ_NOT_PERMITTED: number
  static readonly ATT_WRITE_NOT_PERMITTED: number
  static readonly ATT_INSUFFICIENT_RESOURCES: number
  /** ATT result codes for use with `server.respondToRequest()`. */
  static readonly ATT_UNLIKELY_ERROR: number
}

export default Server
