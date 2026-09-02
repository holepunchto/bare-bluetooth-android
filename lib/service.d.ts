import Characteristic from './characteristic'

export default class Service {
  /**
   * @param uuid - The service's UUID.
   * @param characteristics - The characteristics belonging to the service.
   * @param opts - Options; services are primary unless `primary: false` is set.
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
