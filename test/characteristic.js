const test = require('brittle')
const Characteristic = require('../lib/characteristic')

test('characteristic value defaults to null', (t) => {
  const characteristic = new Characteristic('feaf')

  t.is(characteristic.value, null)
})

test('characteristic takes an initial value', (t) => {
  const value = new Uint8Array([1, 2, 3])
  const characteristic = new Characteristic('feaf', { value })

  t.is(characteristic.value, value)
})

test('a local characteristic accepts a value', (t) => {
  const characteristic = new Characteristic('feaf', { read: true })
  const value = new Uint8Array([4, 5, 6])

  characteristic.value = value

  t.is(characteristic.value, value)
})
