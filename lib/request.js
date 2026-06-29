class ReadRequest {
  constructor(handle, requestId, characteristicUuid, offset) {
    this._handle = handle
    this.requestId = requestId
    this.characteristicUuid = characteristicUuid
    this.offset = offset
  }
}

class WriteRequest {
  constructor(handle, requestId, characteristicUuid, offset, data, responseNeeded) {
    this._handle = handle
    this.requestId = requestId
    this.characteristicUuid = characteristicUuid
    this.offset = offset
    this.data = data
    this.responseNeeded = responseNeeded
  }
}

exports.ReadRequest = ReadRequest
exports.WriteRequest = WriteRequest
