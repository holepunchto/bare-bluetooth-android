import { Duplex } from 'bare-stream'

export default class L2CAPChannel extends Duplex {
  /**
   * @param channelHandle - The native channel handle backing the stream; supplied internally when a channel opens, not usually passed directly.
   */
  constructor(channelHandle: ArrayBuffer)

  /** The Protocol/Service Multiplexer number of the channel. */
  readonly psm: number
  /** The address of the remote peer, or `null`. */
  readonly peer: string | null
}
