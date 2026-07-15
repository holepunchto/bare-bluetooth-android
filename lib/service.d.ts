import Characteristic from './characteristic'

/** Create a new GATT service definition. */
export default class Service {
  /**
   * @param uuid - The service's UUID.
   * @param characteristics - The characteristics belonging to the service.
   * @param opts - Options; set `primary: true` to mark this a primary service.
   */
  constructor(uuid: string, characteristics?: Characteristic[], opts?: ServiceOptions)

  /** The UUID of the service. */
  readonly uuid: string
  /** The array of characteristics belonging to the service. */
  readonly characteristics: Characteristic[]
  /** Whether the service is a primary service. */
  readonly primary: boolean
}

export interface ServiceOptions {
  primary?: boolean
}
