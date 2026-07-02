import { EventEmitter, EventMap } from 'bare-events'
import Service from './service'
import Characteristic from './characteristic'
import L2CAPChannel from './channel'
import BluetoothError from './errors'

export type BluetoothState = 'off' | 'turningOn' | 'on' | 'turningOff'

export interface AdvertisingOptions {
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
  subscribe: [deviceAddress: string, characteristicUuid: string]
  unsubscribe: [deviceAddress: string, characteristicUuid: string]
  connecting: [deviceAddress: string]
  connected: [deviceAddress: string]
  disconnecting: [deviceAddress: string]
  disconnected: [deviceAddress: string]
  notifySent: [deviceAddress: string, status: number]
  error: [error: BluetoothError]
}

declare class Server extends EventEmitter<ServerEventMap> {
  constructor()

  readonly state: BluetoothState

  addService(service: Service): void
  startAdvertising(opts?: AdvertisingOptions): void
  stopAdvertising(): void
  respondToRequest(request: ReadRequest, result: number, data?: Uint8Array): void
  updateValue(characteristic: Characteristic, data: Uint8Array): boolean
  publishChannel(opts?: ChannelOptions): void
  unpublishChannel(psm: number): void
  destroy(): void

  static readonly STATE_OFF: number
  static readonly STATE_TURNING_ON: number
  static readonly STATE_ON: number
  static readonly STATE_TURNING_OFF: number

  static readonly PROPERTY_READ: number
  static readonly PROPERTY_WRITE_WITHOUT_RESPONSE: number
  static readonly PROPERTY_WRITE: number
  static readonly PROPERTY_NOTIFY: number
  static readonly PROPERTY_INDICATE: number

  static readonly PERMISSION_READABLE: number
  static readonly PERMISSION_WRITEABLE: number
  static readonly PERMISSION_READ_ENCRYPTED: number
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
  static readonly ATT_UNLIKELY_ERROR: number
}

export default Server
