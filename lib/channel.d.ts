import { Duplex } from 'bare-stream'

/** A duplex stream representing an L2CAP connection-oriented channel. Extends `Duplex` from <https://github.com/holepunchto/bare-stream>. Typically obtained via the `'channelOpen'` event rather than constructed directly. */
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
