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

## Errors

Failures arrive two ways:

- **Thrown** - the Android call itself failed. The message carries the Java exception, for example `java.lang.IllegalStateException: BT Adapter is not turned ON`. Wrap the call in `try`/`catch`.
- **Emitted** - the call went through and the operation failed later. The `error` event carries a `BluetoothError` with a `code` such as `SCAN_FAILED` or `CONNECTION_FAILED`.

## API

See the [`bare-bluetooth-android` reference](https://docs.pears.com/reference/bare/modules/bare-bluetooth-android).

## License

Apache-2.0
