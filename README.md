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

## API

#### `const central = new Central()`

Create a new BLE central manager for scanning and connecting to peripherals.

#### `central.state`

The current Bluetooth adapter state. One of `'off'`, `'turningOn'`, `'on'`, or `'turningOff'`.

#### `central.startScan(serviceUUIDs[, opts])`

Start scanning for peripherals advertising the given `serviceUUIDs`. Pass `null` to scan for all peripherals.

Options include:

```js
opts = {
  scanMode: null
}
```

Set `scanMode` to one of `Central.SCAN_MODE_OPPORTUNISTIC`, `Central.SCAN_MODE_LOW_POWER`, `Central.SCAN_MODE_BALANCED`, or `Central.SCAN_MODE_LOW_LATENCY`.

#### `central.stopScan()`

Stop scanning for peripherals.

#### `central.connect(peripheral)`

Connect to a discovered `peripheral`.

#### `central.disconnect(peripheral)`

Disconnect from a connected `peripheral`.

#### `central.destroy()`

Destroy the central manager, disconnecting all connected peripherals.

#### `event: 'stateChange'`

Emitted when the Bluetooth adapter state changes. The listener receives the new `state` string.

#### `event: 'discover'`

Emitted when a peripheral is discovered during scanning. The listener receives a `peripheral` object with `handle`, `id`, `name`, and `rssi` properties.

#### `event: 'connect'`

Emitted when a connection to a peripheral is established. The listener receives a `Peripheral` instance.

#### `event: 'disconnect'`

Emitted when a peripheral disconnects. The listener receives the `peripheral` and an optional `error`.

#### `event: 'connectFail'`

Emitted when a connection attempt fails. The listener receives the peripheral `id` and an `error`.

#### `event: 'scanFail'`

Emitted when scanning fails. The listener receives the `errorCode`.

#### `Central.SCAN_MODE_OPPORTUNISTIC`

#### `Central.SCAN_MODE_LOW_POWER`

#### `Central.SCAN_MODE_BALANCED`

#### `Central.SCAN_MODE_LOW_LATENCY`

Scan mode constants for use with `central.startScan()`.

#### `const peripheral = new Peripheral(peripheralHandle[, opts])`

Create a new peripheral instance. Typically obtained via the `'connect'` event on `Central` rather than constructed directly.

Options include:

```js
opts = {
  connectHandle: null,
  id: null,
  name: null
}
```

#### `peripheral.id`

The unique identifier of the peripheral.

#### `peripheral.name`

The advertised name of the peripheral, or `null` if unavailable.

#### `peripheral.discoverServices()`

Discover services offered by the peripheral. Results are emitted via the `'servicesDiscover'` event.

#### `peripheral.discoverCharacteristics(service)`

Discover characteristics for the given `service`. Results are emitted via the `'characteristicsDiscover'` event.

#### `peripheral.read(characteristic)`

Read the value of `characteristic`. The result is emitted via the `'read'` event.

#### `peripheral.write(characteristic, data[, withResponse])`

Write `data` to `characteristic`. If `withResponse` is `true` (the default), a write confirmation is requested.

#### `peripheral.subscribe(characteristic)`

Subscribe to notifications for `characteristic`.

#### `peripheral.unsubscribe(characteristic)`

Unsubscribe from notifications for `characteristic`.

#### `peripheral.openL2CAPChannel(psm)`

Open an L2CAP channel to the peripheral using the given `psm`. The result is emitted via the `'channelOpen'` event.

#### `peripheral.requestMtu(mtu)`

Request a new MTU size. The result is emitted via the `'mtuChanged'` event.

#### `peripheral.destroy()`

Destroy the peripheral instance.

#### `event: 'servicesDiscover'`

Emitted when services are discovered. The listener receives an array of `Service` instances and an optional `error`.

#### `event: 'characteristicsDiscover'`

Emitted when characteristics are discovered. The listener receives the `service`, an array of `Characteristic` instances, and an optional `error`.

#### `event: 'read'`

Emitted when a characteristic read completes. The listener receives the `characteristic`, `data`, and an optional `error`.

#### `event: 'write'`

Emitted when a characteristic write completes. The listener receives the `characteristic` and an optional `error`.

#### `event: 'notify'`

Emitted when a characteristic notification is received. The listener receives the `characteristic`, `data`, and an optional `error`.

#### `event: 'notifyState'`

Emitted when the notification state changes. The listener receives the `characteristic`, `isNotifying`, and an optional `error`.

#### `event: 'channelOpen'`

Emitted when an L2CAP channel is opened. The listener receives an `L2CAPChannel` instance or `null`, and an optional `error`.

#### `event: 'mtuChanged'`

Emitted when the MTU is changed. The listener receives the new `mtu` and an optional `error`.

#### `Peripheral.PROPERTY_READ`

#### `Peripheral.PROPERTY_WRITE_WITHOUT_RESPONSE`

#### `Peripheral.PROPERTY_WRITE`

#### `Peripheral.PROPERTY_NOTIFY`

#### `Peripheral.PROPERTY_INDICATE`

Characteristic property constants.

#### `const server = new Server()`

Create a new BLE peripheral server for advertising services and handling client requests.

#### `server.state`

The current Bluetooth adapter state. One of `'off'`, `'turningOn'`, `'on'`, or `'turningOff'`.

#### `server.addService(service)`

Add a `service` to the GATT server. The `'serviceAdd'` event is emitted when the service has been registered.

#### `server.startAdvertising([opts])`

Start advertising the server.

Options include:

```js
opts = {
  name: null,
  serviceUUIDs: null
}
```

#### `server.stopAdvertising()`

Stop advertising.

#### `server.respondToRequest(request, result[, data])`

Respond to a read or write `request` with a `result` code and optional `data`. Use the `Server.ATT_*` constants for the result.

#### `server.updateValue(characteristic, data)`

Update the value of `characteristic` with `data` and notify subscribed clients. Returns `true` if the notification was sent successfully.

#### `server.publishChannel([opts])`

Publish an L2CAP channel. The `'channelPublish'` event is emitted with the assigned PSM.

Options include:

```js
opts = {
  encrypted: false
}
```

#### `server.unpublishChannel(psm)`

Unpublish an L2CAP channel with the given `psm`.

#### `server.destroy()`

Destroy the server.

#### `event: 'stateChange'`

Emitted when the Bluetooth adapter state changes. The listener receives the new `state` string.

#### `event: 'serviceAdd'`

Emitted when a service is added. The listener receives the `uuid` and an optional `error`.

#### `event: 'readRequest'`

Emitted when a client reads a characteristic. The listener receives a `request` object with `handle`, `requestId`, `characteristicUuid`, and `offset` properties.

#### `event: 'writeRequest'`

Emitted when a client writes to a characteristic. The listener receives an array of request objects, each with `handle`, `requestId`, `characteristicUuid`, `data`, `offset`, and `responseNeeded` properties.

#### `event: 'subscribe'`

Emitted when a client subscribes to notifications. The listener receives `deviceAddress` and `characteristicUuid`.

#### `event: 'unsubscribe'`

Emitted when a client unsubscribes from notifications. The listener receives `deviceAddress` and `characteristicUuid`.

#### `event: 'advertiseError'`

Emitted when advertising fails. The listener receives `errorCode` and `error`.

#### `event: 'channelPublish'`

Emitted when an L2CAP channel is published. The listener receives the `psm` and an optional `error`.

#### `event: 'channelOpen'`

Emitted when an L2CAP channel is opened by a client. The listener receives an `L2CAPChannel` instance or `null`, and an optional `error`.

#### `event: 'notifySent'`

Emitted when a notification is delivered. The listener receives `deviceAddress` and `status`.

#### `Server.PROPERTY_READ`

#### `Server.PROPERTY_WRITE_WITHOUT_RESPONSE`

#### `Server.PROPERTY_WRITE`

#### `Server.PROPERTY_NOTIFY`

#### `Server.PROPERTY_INDICATE`

Characteristic property constants.

#### `Server.PERMISSION_READABLE`

#### `Server.PERMISSION_WRITEABLE`

#### `Server.PERMISSION_READ_ENCRYPTED`

#### `Server.PERMISSION_WRITE_ENCRYPTED`

Characteristic permission constants.

#### `Server.ATT_SUCCESS`

#### `Server.ATT_INVALID_HANDLE`

#### `Server.ATT_READ_NOT_PERMITTED`

#### `Server.ATT_WRITE_NOT_PERMITTED`

#### `Server.ATT_INSUFFICIENT_RESOURCES`

#### `Server.ATT_UNLIKELY_ERROR`

ATT result codes for use with `server.respondToRequest()`.

#### `const channel = new L2CAPChannel(channelHandle)`

A duplex stream representing an L2CAP connection-oriented channel. Extends `Duplex` from <https://github.com/holepunchto/bare-stream>. Typically obtained via the `'channelOpen'` event rather than constructed directly.

#### `channel.psm`

The Protocol/Service Multiplexer number of the channel.

#### `channel.peer`

The address of the remote peer, or `null`.

#### `const service = new Service(uuid[, characteristics][, opts])`

Create a new GATT service definition.

Options include:

```js
opts = {
  primary: true
}
```

#### `service.uuid`

The UUID of the service.

#### `service.characteristics`

The array of characteristics belonging to the service.

#### `service.primary`

Whether the service is a primary service.

#### `const characteristic = new Characteristic(uuid[, opts])`

Create a new GATT characteristic definition.

Options include:

```js
opts = {
  read: false,
  write: false,
  writeWithoutResponse: false,
  notify: false,
  indicate: false,
  permissions: null,
  value: null
}
```

Set `read`, `write`, `writeWithoutResponse`, `notify`, and `indicate` to configure the characteristic properties. If `permissions` is `null`, permissions are inferred from the properties.

#### `characteristic.uuid`

The UUID of the characteristic.

#### `characteristic.properties`

The property flags of the characteristic.

#### `characteristic.permissions`

The permission flags of the characteristic, or `null` if inferred.

#### `characteristic.value`

The current value of the characteristic, or `null`.

#### `Characteristic.PROPERTY_READ`

#### `Characteristic.PROPERTY_WRITE_WITHOUT_RESPONSE`

#### `Characteristic.PROPERTY_WRITE`

#### `Characteristic.PROPERTY_NOTIFY`

#### `Characteristic.PROPERTY_INDICATE`

Characteristic property constants.

## License

Apache-2.0
