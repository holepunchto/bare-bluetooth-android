import { EventEmitter, EventMap } from 'bare-events'
import Service from './service'
import Characteristic from './characteristic'
import L2CAPChannel from './channel'
import ScanResult from './scan-result'
import Device from './device'
import BluetoothError from './errors'
import type { ServiceData } from './scan-record'

export interface PeripheralOptions {
  /** The `ScanResult` from the most recent advertisement for this peripheral. */
  scanResult?: ScanResult
  /** A device to dial without a scan, such as one from `central.getBondedDevices()`. */
  device?: Device
}

export interface PeripheralEventMap extends EventMap {
  servicesDiscover: [services: Service[]]
  characteristicsDiscover: [service: Service | null, characteristics: Characteristic[]]
  /**
   * Emitted when a read completes, carrying the characteristic that was read, or `null` if it is
   * not recognised, and the bytes returned.
   */
  read: [characteristic: Characteristic | null, data: Uint8Array]
  /**
   * Emitted when a write completes, carrying the characteristic that was written, or `null` if it
   * is not recognised.
   */
  write: [characteristic: Characteristic | null]
  notify: [characteristic: Characteristic | null, data: Uint8Array]
  notifyState: [characteristic: Characteristic | null, isNotifying: boolean]
  channelOpen: [channel: L2CAPChannel]
  mtuChanged: [mtu: number]
  /** Emitted when the peripheral has disconnected without error. */
  disconnect: []
  error: [error: BluetoothError]
}

export default class Peripheral extends EventEmitter<PeripheralEventMap> {
  /**
   * @param opts - Options carrying the `ScanResult` or the `Device` this peripheral is derived
   * from.
   */
  constructor(opts: PeripheralOptions)

  /** The most recent advertisement, or `null` when the peripheral was built from a `Device`. */
  readonly scanResult: ScanResult | null
  /** The unique identifier of the peripheral, equal to the device address. */
  readonly id: string
  /** The name of the peripheral, or `null` if unavailable. */
  readonly name: string | null
  /** The signal strength of the most recent advertisement, or `null` without a scan. */
  readonly rssi: number | null

  /** Whether the peripheral is connected */
  readonly connected: boolean

  /** Whether a dial is in flight and has not connected yet */
  readonly connecting: boolean
  /**
   * The advertised service data, or `null` when the advertisement carried no scan record or no
   * service data.
   */
  readonly serviceData: ServiceData | null

  /**
   * Discover services offered by the peripheral. Results are emitted via the `'servicesDiscover'`
   * event.
   */
  discoverServices(): void
  /**
   * @param service - The service to discover characteristics on.
   */
  discoverCharacteristics(service: Service): void
  /**
   * @param characteristic - The characteristic to read.
   */
  read(characteristic: Characteristic): void
  /**
   * @param characteristic - The characteristic to write to.
   * @param data - The bytes to write.
   * @param withResponse - Whether a write confirmation is requested (default `true`).
   */
  write(characteristic: Characteristic, data: Uint8Array, withResponse?: boolean): void
  /**
   * @param characteristic - The characteristic to start receiving notifications for.
   */
  subscribe(characteristic: Characteristic): void
  /**
   * @param characteristic - The characteristic to stop receiving notifications for.
   */
  unsubscribe(characteristic: Characteristic): void
  /**
   * @param psm - The PSM (Protocol/Service Multiplexer) of the channel to open.
   */
  openL2CAPChannel(psm: number): void
  /**
   * @param mtu - The desired ATT MTU size, in bytes.
   */
  requestMtu(mtu: number): void
  /** Destroy the peripheral, releasing its underlying resources. */
  destroy(): void

  static readonly PROPERTY_READ: number
  static readonly PROPERTY_WRITE_WITHOUT_RESPONSE: number
  static readonly PROPERTY_WRITE: number
  static readonly PROPERTY_NOTIFY: number
  /** Characteristic property constants. */
  static readonly PROPERTY_INDICATE: number
}
