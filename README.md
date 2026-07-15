# bare-bluetooth-android

Android Bluetooth Low Energy (BLE) bindings for Bare, providing both central and peripheral roles. Built on the Android Bluetooth API.

```
npm i bare-bluetooth-android
```

## Usage

```js
const { Central } = require('bare-bluetooth-android')

const central = new Central()

central.on('stateChange', (state) => {
  if (state === 'on') {
    central.startScan(['180D']) // Heart Rate service UUID
  }
})

central.on('discover', (peripheral) => {
  console.log('Found:', peripheral.name, peripheral.id)
  central.stopScan()
  central.connect(peripheral)
})

central.on('connect', (peripheral) => {
  peripheral.discoverServices()

  peripheral.on('servicesDiscover', (services) => {
    // Discover characteristics for each service
  })
})
```

<!-- bare-refgen:api start -->
## API

### BluetoothError

#### `new BluetoothError(msg: string, fn?: Function, code?: string)`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/errors.d.ts#L2)

**Parameters**

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| `msg` | `string` | — | A human-readable error message. |
| `fn?` | `Function` | — | The function to omit from the captured stack trace (default `BluetoothError`). |
| `code?` | `string` | — | The error code; defaults to `fn.name` (e.g. `SCAN_FAILED`). |

#### `BluetoothError.ADVERTISE_FAILED(msg: string): BluetoothError`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/errors.d.ts#L8)

**Parameters**

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| `msg` | `string` | — | — |

#### `BluetoothError.CHANNEL_FAILED(msg: string): BluetoothError`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/errors.d.ts#L17)

**Parameters**

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| `msg` | `string` | — | — |

#### `BluetoothError.CHANNEL_PUBLISH_FAILED(msg: string): BluetoothError`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/errors.d.ts#L19)

**Parameters**

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| `msg` | `string` | — | — |

#### `BluetoothError.CONNECTION_FAILED(msg: string, id: string): BluetoothError`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/errors.d.ts#L10)

**Parameters**

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| `msg` | `string` | — | A human-readable error message. |
| `id` | `string` | — | The id of the peripheral the connection attempt targeted. |

#### `BluetoothError.DISCONNECT(msg: string, id: string): BluetoothError`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/errors.d.ts#L11)

**Parameters**

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| `msg` | `string` | — | A human-readable error message. |
| `id` | `string` | — | The id of the peripheral that disconnected. |

#### `BluetoothError.DISCOVER_FAILED(msg: string): BluetoothError`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/errors.d.ts#L12)

**Parameters**

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| `msg` | `string` | — | — |

#### `BluetoothError.MTU_CHANGE_FAILED(msg: string): BluetoothError`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/errors.d.ts#L20)

**Parameters**

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| `msg` | `string` | — | — |

#### `BluetoothError.NOTIFY_FAILED(msg: string): BluetoothError`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/errors.d.ts#L15)

**Parameters**

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| `msg` | `string` | — | — |

#### `BluetoothError.NOTIFY_STATE_FAILED(msg: string): BluetoothError`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/errors.d.ts#L16)

**Parameters**

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| `msg` | `string` | — | — |

#### `BluetoothError.READ_FAILED(msg: string): BluetoothError`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/errors.d.ts#L13)

**Parameters**

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| `msg` | `string` | — | — |

#### `BluetoothError.SCAN_FAILED(msg: string): BluetoothError`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/errors.d.ts#L9)

**Parameters**

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| `msg` | `string` | — | — |

#### `BluetoothError.SERVICE_ADD_FAILED(msg: string): BluetoothError`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/errors.d.ts#L18)

**Parameters**

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| `msg` | `string` | — | — |

#### `BluetoothError.WRITE_FAILED(msg: string): BluetoothError`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/errors.d.ts#L14)

**Parameters**

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| `msg` | `string` | — | — |

#### `code: string`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/errors.d.ts#L4)

#### `BluetoothError.id: string`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/errors.d.ts#L6)

#### `name: 'BluetoothError'`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/errors.d.ts#L5)

### L2CAPChannel

#### `new L2CAPChannel(channelHandle: ArrayBuffer)`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/channel.d.ts#L4)

A duplex stream representing an L2CAP connection-oriented channel. Extends `Duplex` from <https://github.com/holepunchto/bare-stream>. Typically obtained via the `'channelOpen'` event rather than constructed directly.

**Parameters**

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| `channelHandle` | `ArrayBuffer` | — | The native channel handle backing the stream; supplied internally when a channel opens, not usually passed directly. |

#### `peer: string | null`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/channel.d.ts#L7)

The address of the remote peer, or `null`.

#### `psm: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/channel.d.ts#L6)

The Protocol/Service Multiplexer number of the channel.

### Service

#### `new Service(uuid: string, characteristics?: Characteristic[], opts?: ServiceOptions)`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/service.d.ts#L4)

Create a new GATT service definition.

**Parameters**

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| `uuid` | `string` | — | The service's UUID. |
| `characteristics?` | `Characteristic[]` | — | The characteristics belonging to the service. |
| `opts?` | `ServiceOptions` | — | Options; set `primary: true` to mark this a primary service. |

#### `characteristics: Characteristic[]`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/service.d.ts#L7)

The array of characteristics belonging to the service.

#### `primary: boolean`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/service.d.ts#L8)

Whether the service is a primary service.

#### `Service.uuid: string`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/service.d.ts#L6)

The UUID of the service.

### Characteristic

#### `new Characteristic(uuid: string, opts?: CharacteristicOptions)`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/characteristic.d.ts#L2)

Create a new GATT characteristic definition.

**Parameters**

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| `uuid` | `string` | — | The characteristic's UUID. |
| `opts?` | `CharacteristicOptions` | — | Options selecting the characteristic `properties`, `permissions`, and initial `value`. |

#### `Characteristic.PROPERTY_INDICATE: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/characteristic.d.ts#L13)

Characteristic property constants.

#### `Characteristic.PROPERTY_NOTIFY: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/characteristic.d.ts#L12)

#### `Characteristic.PROPERTY_READ: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/characteristic.d.ts#L9)

#### `Characteristic.PROPERTY_WRITE: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/characteristic.d.ts#L11)

#### `Characteristic.PROPERTY_WRITE_WITHOUT_RESPONSE: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/characteristic.d.ts#L10)

#### `permissions: number | null`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/characteristic.d.ts#L6)

The permission flags of the characteristic, or `null` if inferred.

#### `properties: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/characteristic.d.ts#L5)

The property flags of the characteristic.

#### `Characteristic.uuid: string`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/characteristic.d.ts#L4)

The UUID of the characteristic.

#### `value: Uint8Array | null`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/characteristic.d.ts#L7)

The current value of the characteristic, or `null`.

### Server

#### `new Server()`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/server.d.ts#L52)

Create a new BLE peripheral server for advertising services and handling client requests.

#### `addService(service: Service): void`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/server.d.ts#L56)

Add a `service` to the GATT server. The `'serviceAdd'` event is emitted when the service has been registered.

**Parameters**

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| `service` | `Service` | — | The `Service` to register with the GATT server. |

#### `Server.destroy(): void`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/server.d.ts#L63)

Destroy the server, stopping advertising and unpublishing all L2CAP channels.

#### `publishChannel(opts?: ChannelOptions): void`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/server.d.ts#L61)

Publish an L2CAP channel. The `'channelPublish'` event is emitted with the assigned PSM.

**Parameters**

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| `opts?` | `ChannelOptions` | — | Options for the L2CAP channel to publish. |

#### `respondToRequest(request: ReadRequest, result: number, data?: Uint8Array): void`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/server.d.ts#L59)

Respond to a read or write `request` with a `result` code and optional `data`. Use the `Server.ATT_*` constants for the result.

**Parameters**

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| `request` | `ReadRequest` | — | The read or write request to respond to. |
| `result` | `number` | — | The ATT result code; use the `Server.ATT_*` constants. |
| `data?` | `Uint8Array` | — | The value to return for a read request; omit for write responses. |

#### `Server.ATT_INSUFFICIENT_RESOURCES: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/server.d.ts#L90)

#### `Server.ATT_INVALID_HANDLE: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/server.d.ts#L87)

#### `Server.ATT_READ_NOT_PERMITTED: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/server.d.ts#L88)

#### `Server.ATT_SUCCESS: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/server.d.ts#L86)

#### `Server.ATT_UNLIKELY_ERROR: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/server.d.ts#L91)

ATT result codes for use with `server.respondToRequest()`.

#### `Server.ATT_WRITE_NOT_PERMITTED: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/server.d.ts#L89)

#### `Server.CONNECTION_STATE_CONNECTED: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/server.d.ts#L83)

#### `Server.CONNECTION_STATE_CONNECTING: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/server.d.ts#L82)

#### `Server.CONNECTION_STATE_DISCONNECTED: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/server.d.ts#L81)

#### `Server.CONNECTION_STATE_DISCONNECTING: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/server.d.ts#L84)

#### `Server.PERMISSION_READ_ENCRYPTED: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/server.d.ts#L78)

#### `Server.PERMISSION_READABLE: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/server.d.ts#L76)

#### `Server.PERMISSION_WRITE_ENCRYPTED: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/server.d.ts#L79)

Characteristic permission constants.

#### `Server.PERMISSION_WRITEABLE: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/server.d.ts#L77)

#### `Server.PROPERTY_INDICATE: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/server.d.ts#L74)

Characteristic property constants.

#### `Server.PROPERTY_NOTIFY: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/server.d.ts#L73)

#### `Server.PROPERTY_READ: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/server.d.ts#L70)

#### `Server.PROPERTY_WRITE: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/server.d.ts#L72)

#### `Server.PROPERTY_WRITE_WITHOUT_RESPONSE: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/server.d.ts#L71)

#### `Server.STATE_OFF: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/server.d.ts#L65)

#### `Server.STATE_ON: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/server.d.ts#L67)

#### `Server.STATE_TURNING_OFF: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/server.d.ts#L68)

#### `Server.STATE_TURNING_ON: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/server.d.ts#L66)

#### `startAdvertising(opts?: AdvertisingOptions): void`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/server.d.ts#L57)

Start advertising the server.

**Parameters**

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| `opts?` | `AdvertisingOptions` | — | Advertising options such as the local `name` and the `serviceUUIDs` to advertise. |

#### `Server.state: BluetoothState`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/server.d.ts#L54)

The current Bluetooth adapter state. One of `'off'`, `'turningOn'`, `'on'`, or `'turningOff'`.

#### `stopAdvertising(): void`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/server.d.ts#L58)

Stop advertising.

#### `unpublishChannel(psm: number): void`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/server.d.ts#L62)

Unpublish an L2CAP channel with the given `psm`.

**Parameters**

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| `psm` | `number` | — | The PSM of the channel to unpublish, as assigned when it was published. |

#### `updateValue(characteristic: Characteristic, data: Uint8Array): boolean`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/server.d.ts#L60)

Update the value of `characteristic` with `data` and notify subscribed clients.

**Parameters**

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| `characteristic` | `Characteristic` | — | The characteristic whose value changed. |
| `data` | `Uint8Array` | — | The new value to send to subscribed clients. |

**Returns** `boolean` — Whether the notification was sent to subscribed clients successfully.

### Central

#### `new Central()`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/central.d.ts#L16)

Create a new BLE central manager for scanning and connecting to peripherals.

#### `Central.SCAN_MODE_BALANCED: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/central.d.ts#L33)

#### `Central.SCAN_MODE_LOW_LATENCY: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/central.d.ts#L34)

Scan mode constants for use with `central.startScan()`.

#### `Central.SCAN_MODE_LOW_POWER: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/central.d.ts#L32)

#### `Central.SCAN_MODE_OPPORTUNISTIC: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/central.d.ts#L31)

#### `Central.STATE_OFF: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/central.d.ts#L26)

#### `Central.STATE_ON: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/central.d.ts#L28)

#### `Central.STATE_TURNING_OFF: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/central.d.ts#L29)

#### `Central.STATE_TURNING_ON: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/central.d.ts#L27)

#### `connect(peripheral: Peripheral): void`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/central.d.ts#L22)

Connect to a discovered `peripheral`.

**Parameters**

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| `peripheral` | `Peripheral` | — | A discovered peripheral to connect to. |

#### `Central.destroy(): void`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/central.d.ts#L24)

Destroy the central manager, stopping any active scan and disconnecting all connected peripherals.

#### `disconnect(peripheral: Peripheral): void`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/central.d.ts#L23)

Disconnect from a connected `peripheral`.

**Parameters**

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| `peripheral` | `Peripheral` | — | The connected peripheral to disconnect from. |

#### `startScan(serviceUUIDs?: string[], opts?: { scanMode?: number }): void`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/central.d.ts#L20)

Start scanning for peripherals advertising the given `serviceUUIDs`. Pass `null` to scan for all peripherals.

**Parameters**

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| `serviceUUIDs?` | `string[]` | — | The service UUIDs to filter advertisements by; pass `null` to scan for all peripherals. |
| `opts?` | `{ scanMode?: number }` | — | Options; `scanMode` selects the Android scan mode (one of the `Central.SCAN_MODE_*` constants). |

#### `Central.state: BluetoothState`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/central.d.ts#L18)

The current Bluetooth adapter state. One of `'off'`, `'turningOn'`, `'on'`, or `'turningOff'`.

#### `stopScan(): void`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/central.d.ts#L21)

Stop scanning for peripherals.

### Peripheral

#### `new Peripheral(opts: PeripheralOptions)`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/peripheral.d.ts#L27)

Create a new peripheral instance from a `ScanResult`. Typically obtained via the `'discover'` event on `Central` rather than constructed directly. Its identity and advertised metadata are derived from the scan result.

**Parameters**

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| `opts` | `PeripheralOptions` | — | Options carrying the `ScanResult` this peripheral is derived from. |

#### `Peripheral.destroy(): void`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/peripheral.d.ts#L43)

Destroy the peripheral, releasing its underlying resources.

#### `discoverCharacteristics(service: Service): void`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/peripheral.d.ts#L36)

Discover characteristics for the given `service`. Results are emitted via the `'characteristicsDiscover'` event.

**Parameters**

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| `service` | `Service` | — | The service to discover characteristics on. |

#### `discoverServices(): void`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/peripheral.d.ts#L35)

Discover services offered by the peripheral. Results are emitted via the `'servicesDiscover'` event.

#### `Peripheral.id: string`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/peripheral.d.ts#L30)

The unique identifier of the peripheral, equal to `scanResult.device.address`.

#### `name: string | null`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/peripheral.d.ts#L31)

The advertised name of the peripheral, or `null` if unavailable. Equal to `scanResult.device.name`.

#### `openL2CAPChannel(psm: number): void`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/peripheral.d.ts#L41)

Open an L2CAP channel to the peripheral using the given `psm`. The result is emitted via the `'channelOpen'` event.

**Parameters**

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| `psm` | `number` | — | The PSM (Protocol/Service Multiplexer) of the channel to open. |

#### `Peripheral.PROPERTY_INDICATE: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/peripheral.d.ts#L49)

Characteristic property constants.

#### `Peripheral.PROPERTY_NOTIFY: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/peripheral.d.ts#L48)

#### `Peripheral.PROPERTY_READ: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/peripheral.d.ts#L45)

#### `Peripheral.PROPERTY_WRITE: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/peripheral.d.ts#L47)

#### `Peripheral.PROPERTY_WRITE_WITHOUT_RESPONSE: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/peripheral.d.ts#L46)

#### `read(characteristic: Characteristic): void`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/peripheral.d.ts#L37)

Read the value of `characteristic`. The result is emitted via the `'read'` event.

**Parameters**

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| `characteristic` | `Characteristic` | — | The characteristic to read. |

#### `requestMtu(mtu: number): void`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/peripheral.d.ts#L42)

Request a new MTU size. The result is emitted via the `'mtuChanged'` event.

**Parameters**

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| `mtu` | `number` | — | The desired ATT MTU size, in bytes. |

#### `rssi: number`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/peripheral.d.ts#L32)

The signal strength of the most recent advertisement, equal to `scanResult.rssi`.

#### `scanResult: ScanResult`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/peripheral.d.ts#L29)

The `ScanResult` from the most recent advertisement for this peripheral.

#### `serviceData: ServiceData | null`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/peripheral.d.ts#L33)

The advertised service data, or `null` when the advertisement carried no scan record or no service data.

#### `subscribe(characteristic: Characteristic): void`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/peripheral.d.ts#L39)

Subscribe to notifications for `characteristic`.

**Parameters**

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| `characteristic` | `Characteristic` | — | The characteristic to start receiving notifications for. |

#### `unsubscribe(characteristic: Characteristic): void`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/peripheral.d.ts#L40)

Unsubscribe from notifications for `characteristic`.

**Parameters**

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| `characteristic` | `Characteristic` | — | The characteristic to stop receiving notifications for. |

#### `write(characteristic: Characteristic, data: Uint8Array, withResponse?: boolean): void`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/peripheral.d.ts#L38)

Write `data` to `characteristic`. If `withResponse` is `true` (the default), a write confirmation is requested.

**Parameters**

| Parameter | Type | Default | Description |
| --- | --- | --- | --- |
| `characteristic` | `Characteristic` | — | The characteristic to write to. |
| `data` | `Uint8Array` | — | The bytes to write. |
| `withResponse?` | `boolean` | — | Whether a write confirmation is requested (default `true`). |

### Types

#### `ServiceOptions`

```ts
interface ServiceOptions {
  primary?: boolean
}
```

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/service.d.ts#L11)

#### `CharacteristicOptions`

```ts
interface CharacteristicOptions {
  read?: boolean
  write?: boolean
  writeWithoutResponse?: boolean
  notify?: boolean
  indicate?: boolean
  permissions?: number
  value?: Uint8Array | null
}
```

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/characteristic.d.ts#L16)

#### `BluetoothState`

```ts
type BluetoothState = 'off' | 'turningOn' | 'on' | 'turningOff'
```

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/server.d.ts#L7)

#### `AdvertisingOptions`

```ts
interface AdvertisingOptions {
  name?: string
  serviceUUIDs?: string[]
}
```

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/server.d.ts#L9)

#### `ChannelOptions`

```ts
interface ChannelOptions {
  encrypted?: boolean
}
```

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/server.d.ts#L14)

#### `ReadRequest`

```ts
interface ReadRequest {
  handle: unknown
  requestId: number
  characteristicUuid: string
  offset: number
}
```

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/server.d.ts#L18)

#### `WriteRequest`

```ts
interface WriteRequest {
  handle: unknown
  requestId: number
  characteristicUuid: string
  data: Uint8Array
  offset: number
  responseNeeded: boolean
}
```

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/server.d.ts#L25)

#### `PeripheralOptions`

```ts
interface PeripheralOptions {
  scanResult: ScanResult
}
```

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/peripheral.d.ts#L9)

#### `ServiceData`

```ts
type ServiceData = {
  [uuid: string]: Uint8Array
}
```

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/scan-record.d.ts#L1)

### Classes

#### `ScanResult`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/scan-result.d.ts#L4)

```ts
class ScanResult {
  device: Device
  rssi: number
  scanRecord: ScanRecord | null
}
```

#### `ScanRecord`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/scan-record.d.ts#L5)

```ts
class ScanRecord {
  serviceData: ServiceData | null
}
```

#### `Device`

[source](https://github.com/holepunchto/bare-bluetooth-android/blob/v0.5.1/lib/device.d.ts#L1)

```ts
class Device {
  address: string
  name: string | null
}
```
<!-- bare-refgen:api end -->

## License

Apache-2.0
