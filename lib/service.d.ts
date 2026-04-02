import Characteristic from './characteristic'

export default class Service {
  constructor(uuid: string, characteristics?: Characteristic[], opts?: ServiceOptions)

  readonly uuid: string
  readonly characteristics: Characteristic[]
  readonly primary: boolean
}

export interface ServiceOptions {
  primary?: boolean
}
