import { EventEmitter, EventMap } from 'bare-events'
import Service from './service'
import Characteristic from './characteristic'
import L2CAPChannel from './channel'
import ScanResult from './scan-result'
import BluetoothError from './errors'
import type { ServiceData } from './scan-record'

export interface PeripheralOptions {
  scanResult: ScanResult
}

export interface PeripheralEventMap extends EventMap {
  servicesDiscover: [services: Service[]]
  characteristicsDiscover: [service: Service | null, characteristics: Characteristic[]]
  read: [characteristic: Characteristic | null, data: Uint8Array]
  write: [characteristic: Characteristic | null]
  notify: [characteristic: Characteristic | null, data: Uint8Array]
  notifyState: [characteristic: Characteristic | null, isNotifying: boolean]
  channelOpen: [channel: L2CAPChannel]
  mtuChanged: [mtu: number]
  disconnect: []
  error: [error: BluetoothError]
}

export default class Peripheral extends EventEmitter<PeripheralEventMap> {
  constructor(opts: PeripheralOptions)

  readonly scanResult: ScanResult
  readonly id: string
  readonly name: string | null
  readonly rssi: number
  readonly serviceData: ServiceData | null

  discoverServices(): void
  discoverCharacteristics(service: Service): void
  read(characteristic: Characteristic): void
  write(characteristic: Characteristic, data: Uint8Array, withResponse?: boolean): void
  subscribe(characteristic: Characteristic): void
  unsubscribe(characteristic: Characteristic): void
  openL2CAPChannel(psm: number): void
  requestMtu(mtu: number): void
  destroy(): void

  static readonly PROPERTY_READ: number
  static readonly PROPERTY_WRITE_WITHOUT_RESPONSE: number
  static readonly PROPERTY_WRITE: number
  static readonly PROPERTY_NOTIFY: number
  static readonly PROPERTY_INDICATE: number
}
