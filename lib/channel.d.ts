import { Duplex } from 'bare-stream'

export default class L2CAPChannel extends Duplex {
  constructor(channelHandle: ArrayBuffer)

  readonly psm: number
  readonly peer: string | null
}
