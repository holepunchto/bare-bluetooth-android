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

Failures arrive three ways:

- **Thrown** - the Android call itself failed. The message carries the Java exception, for example `java.lang.IllegalStateException: BT Adapter is not turned ON`. Wrap the call in `try`/`catch`.
- **Emitted** - the call went through and the operation failed later. The `error` event carries a `BluetoothError` with a `code` such as `SCAN_FAILED` or `CONNECTION_FAILED`.
- **Streamed** - an `L2CAPChannel` is a `Duplex`, so a failed write does not throw from the write itself. It destroys the stream with the Java exception, for example `java.io.IOException: Broken pipe` once the peer is gone. Listen for `error` on the channel; an unhandled one still throws.

## Testing on device

The tests drive a real radio, so they only run on a phone. Plug in an arm64 device with USB debugging on.

Build the addon for the device, which needs an Android NDK:

```console
bare-make generate --platform android --arch arm64 -D ANDROID_STL=c++_static
bare-make build
bare-make install
```

Then run the tests:

```console
npm run test:device
```

`test:device` hands `test.js` to [`bare-native-test`](https://github.com/holepunchto/bare-native-test), which builds it into an app, installs it with the Bluetooth permissions already granted, launches it and streams the TAP output. [`test/AndroidManifest.xml`](test/AndroidManifest.xml) is the template that declares those permissions.

### The state suite

```console
sh test/state.sh &
npx bare-native-test --platform android --runtime bare-ndk/runtime --android-manifest test/AndroidManifest.xml test-state.js
```

`test-state.js` adds `test/state.js`, and [`state.sh`](test/state.sh) toggles the radio for it to observe.

## API

See the [`bare-bluetooth-android` reference](https://docs.pears.com/reference/bare/modules/bare-bluetooth-android).

## License

Apache-2.0
