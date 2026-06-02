import { EventEmitter, EventMap } from 'bare-events'
import Peripheral from './peripheral'

export type BluetoothState = 'off' | 'turningOn' | 'on' | 'turningOff'

export interface CentralEventMap extends EventMap {
  stateChange: [state: BluetoothState]
  discover: [peripheral: Peripheral]
  connect: [peripheral: Peripheral, error?: string]
  disconnect: [peripheral: Peripheral | null, error?: string]
  connectFail: [id: string, error: string]
  scanFail: [errorCode: number]
}

export default class Central extends EventEmitter<CentralEventMap> {
  constructor()

  readonly state: BluetoothState

  startScan(serviceUUIDs?: string[]): void
  stopScan(): void
  connect(peripheral: Peripheral): void
  disconnect(peripheral: Peripheral): void
  destroy(): void

  static readonly STATE_OFF: number
  static readonly STATE_TURNING_ON: number
  static readonly STATE_ON: number
  static readonly STATE_TURNING_OFF: number
}
