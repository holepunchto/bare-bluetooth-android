import { EventEmitter, EventMap } from 'bare-events'
import Service from './service'
import Characteristic from './characteristic'
import L2CAPChannel from './channel'
import ScanResult from './scan-result'
import BluetoothError from './errors'
import type { ServiceData } from './scan-record'

export interface PeripheralOptions {
  /** The `ScanResult` from the most recent advertisement for this peripheral. */
  scanResult: ScanResult
}

export interface PeripheralEventMap extends EventMap {
  servicesDiscover: [services: Service[]]
  characteristicsDiscover: [service: Service | null, characteristics: Characteristic[]]
  /**
   * Read the value of `characteristic`. The result is emitted via the `'read'` event.
   * @param characteristic - The characteristic to read.
   */
  read: [characteristic: Characteristic | null, data: Uint8Array]
  /**
   * Write `data` to `characteristic`. If `withResponse` is `true` (the default), a write confirmation is requested.
   * @param characteristic - The characteristic to write to.
   * @param data - The bytes to write.
   * @param withResponse - Whether a write confirmation is requested (default `true`).
   */
  write: [characteristic: Characteristic | null]
  notify: [characteristic: Characteristic | null, data: Uint8Array]
  notifyState: [characteristic: Characteristic | null, isNotifying: boolean]
  channelOpen: [channel: L2CAPChannel]
  mtuChanged: [mtu: number]
  /**
   * Disconnect from a connected `peripheral`.
   * @param peripheral - The connected peripheral to disconnect from.
   */
  disconnect: []
  error: [error: BluetoothError]
}

/** Create a new peripheral instance from a `ScanResult`. Typically obtained via the `'discover'` event on `Central` rather than constructed directly. Its identity and advertised metadata are derived from the scan result. */
export default class Peripheral extends EventEmitter<PeripheralEventMap> {
  /**
   * @param opts - Options carrying the `ScanResult` this peripheral is derived from.
   */
  constructor(opts: PeripheralOptions)

  readonly scanResult: ScanResult
  /** The unique identifier of the peripheral, equal to `scanResult.device.address`. */
  readonly id: string
  /** The advertised name of the peripheral, or `null` if unavailable. Equal to `scanResult.device.name`. */
  readonly name: string | null
  /** The signal strength of the most recent advertisement, equal to `scanResult.rssi`. */
  readonly rssi: number
  /** The advertised service data, or `null` when the advertisement carried no scan record or no service data. */
  readonly serviceData: ServiceData | null

  /** Discover services offered by the peripheral. Results are emitted via the `'servicesDiscover'` event. */
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
