const test = require('brittle')
const Device = require('../lib/device')
const ScanRecord = require('../lib/scan-record')
const ScanResult = require('../lib/scan-result')

test('device exposes address and name', (t) => {
  const device = new Device({ address: 'AA:BB:CC:DD:EE:FF', name: 'sensor' })

  t.is(device.address, 'AA:BB:CC:DD:EE:FF')
  t.is(device.name, 'sensor')
})

test('device name is null when absent', (t) => {
  const device = new Device({ address: 'AA:BB:CC:DD:EE:FF', name: null })

  t.is(device.name, null)
})

test('scan record exposes service data', (t) => {
  const serviceData = { feaf: new Uint8Array([1, 2, 3]) }
  const record = new ScanRecord({ serviceData })

  t.is(record.serviceData, serviceData)
})

test('scan record service data is null when absent', (t) => {
  const record = new ScanRecord({ serviceData: null })

  t.is(record.serviceData, null)
})

test('scan result exposes device, rssi, and scan record', (t) => {
  const device = new Device({ address: 'AA:BB:CC:DD:EE:FF', name: 'sensor' })
  const record = new ScanRecord({ serviceData: null })
  const result = new ScanResult({ device, rssi: -42, scanRecord: record })

  t.is(result.device, device)
  t.is(result.rssi, -42)
  t.is(result.scanRecord, record)
})

test('scan result scan record is null when absent', (t) => {
  const device = new Device({ address: 'AA:BB:CC:DD:EE:FF', name: null })
  const result = new ScanResult({ device, rssi: -50, scanRecord: null })

  t.is(result.scanRecord, null)
})
