import { EventEmitter, EventMap } from 'bare-events'
import Service from './service'
import Characteristic from './characteristic'
import L2CAPChannel from './channel'
import ScanResult from './scan-result'
import type { ServiceData } from './scan-record'

export interface PeripheralOptions {
  scanResult: ScanResult
}

export interface PeripheralEventMap extends EventMap {
  servicesDiscover: [services: Service[] | null, error?: string]
  characteristicsDiscover: [
    service: string,
    characteristics: Characteristic[] | null,
    error?: string
  ]
  read: [characteristic: string, data: Uint8Array, error?: string]
  write: [characteristic: string, error?: string]
  notify: [characteristic: string, data: Uint8Array, error?: string]
  notifyState: [characteristic: string, isNotifying: boolean, error?: string]
  channelOpen: [channel: L2CAPChannel | null, error?: string]
  mtuChanged: [mtu: number, error?: string]
}

export default class Peripheral extends EventEmitter<PeripheralEventMap> {
  constructor(opts: PeripheralOptions)

  readonly scanResult: ScanResult
  readonly id: string
  readonly name: string | null
  readonly rssi: number
  readonly serviceData: ServiceData | null

  discoverServices(): void
  discoverCharacteristics(service: string): void
  read(characteristic: string): void
  write(characteristic: string, data: Uint8Array, withResponse?: boolean): void
  subscribe(characteristic: string): void
  unsubscribe(characteristic: string): void
  openL2CAPChannel(psm: number): void
  requestMtu(mtu: number): void
  destroy(): void

  static readonly PROPERTY_READ: number
  static readonly PROPERTY_WRITE_WITHOUT_RESPONSE: number
  static readonly PROPERTY_WRITE: number
  static readonly PROPERTY_NOTIFY: number
  static readonly PROPERTY_INDICATE: number
}
