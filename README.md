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

## Testing on device

The tests drive a real radio, so they only run on a phone. `test/test-app/` is a minimal Android app that boots a Bare worklet running `test.js` and prints the TAP output on screen.

```console
npm run setup:android
npm run test:android
```

The first command downloads the latest Bare Kit prebuild and compiles the addon for `android-arm64`; it needs the [GitHub CLI](https://cli.github.com) and an Android NDK. Re-run it after touching `binding.cc`.

If a firewall such as Socket blocks `api.github.com` for anything npm spawns, run `sh test/test-app/setup.sh` directly instead.

The second builds the APK, installs it, launches it and tails the TAP output. Plug in an arm64 device with USB debugging on, and grant the Bluetooth permissions on first launch.

## API

See the [`bare-bluetooth-android` reference](https://docs.pears.com/reference/bare/modules/bare-bluetooth-android).

## License

Apache-2.0
