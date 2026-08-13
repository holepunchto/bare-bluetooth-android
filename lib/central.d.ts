import { EventEmitter, EventMap } from 'bare-events'
import Peripheral from './peripheral'
import BluetoothError from './errors'

export type BluetoothState = 'off' | 'turningOn' | 'on' | 'turningOff'

export interface CentralEventMap extends EventMap {
  stateChange: [state: BluetoothState]
  discover: [peripheral: Peripheral]
  /** Emitted when a connection is established, with the connected `Peripheral`. */
  connect: [peripheral: Peripheral]
  /**
   * Emitted when a peripheral disconnects, with the disconnected `Peripheral`, or `null` if it
   * was not tracked as connected. If the disconnect carried an error, `error` is emitted instead.
   */
  disconnect: [peripheral: Peripheral | null]
  error: [error: BluetoothError]
}

export default class Central extends EventEmitter<CentralEventMap> {
  /** Create a new BLE central manager for scanning and connecting to peripherals. */
  constructor()

  /**
   * The current Bluetooth adapter state. One of `'off'`, `'turningOn'`, `'on'`, or `'turningOff'`.
   */
  readonly state: BluetoothState

  /**
   * @param serviceUUIDs - The service UUIDs to filter advertisements by; pass `null` to scan for
   * all peripherals.
   * @param opts - Options; `scanMode` selects the Android scan mode (one of the
   * `Central.SCAN_MODE_*` constants).
   */
  startScan(serviceUUIDs?: string[], opts?: { scanMode?: number; callbackType?: number }): void
  /** Stop scanning for peripherals. */
  stopScan(): void
  /**
   * Connect to a discovered `peripheral`.
   * @param peripheral - A discovered peripheral to connect to.
   */
  connect(peripheral: Peripheral): void
  /**
   * Disconnect from a connected `peripheral`.
   * @param peripheral - The connected peripheral to disconnect from.
   */
  disconnect(peripheral: Peripheral): void
  /**
   * Destroy the central manager, stopping any active scan and destroying every connected
   * peripheral.
   */
  destroy(): void

  static readonly STATE_OFF: number
  static readonly STATE_TURNING_ON: number
  static readonly STATE_ON: number
  static readonly STATE_TURNING_OFF: number

  static readonly SCAN_MODE_OPPORTUNISTIC: number
  static readonly SCAN_MODE_LOW_POWER: number
  static readonly SCAN_MODE_BALANCED: number
  /** Scan mode constants for use with `central.startScan()`. */
  static readonly SCAN_MODE_LOW_LATENCY: number

  static readonly CALLBACK_TYPE_ALL_MATCHES: number
  static readonly CALLBACK_TYPE_FIRST_MATCH: number
  static readonly CALLBACK_TYPE_MATCH_LOST: number
}
