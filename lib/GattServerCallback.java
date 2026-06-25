package to.holepunch.bare.bluetooth;

import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattServer;
import android.bluetooth.BluetoothGattService;

public final class GattServerCallback extends android.bluetooth.BluetoothGattServerCallback {
  private final long nativePointer;

  private long tsfnConnectionState;
  private long tsfnAddService;
  private long tsfnReadRequest;
  private long tsfnWriteRequest;
  private long tsfnDescriptorResponse;
  private long tsfnSubscribe;
  private long tsfnUnsubscribe;
  private long tsfnNotifySent;

  public GattServerCallback(long nativePointer) {
    this.nativePointer = nativePointer;
  }

  public void
  setTsfns(long tsfnConnectionState, long tsfnAddService, long tsfnReadRequest, long tsfnWriteRequest, long tsfnDescriptorResponse, long tsfnSubscribe, long tsfnUnsubscribe, long tsfnNotifySent) {
    this.tsfnConnectionState = tsfnConnectionState;
    this.tsfnAddService = tsfnAddService;
    this.tsfnReadRequest = tsfnReadRequest;
    this.tsfnWriteRequest = tsfnWriteRequest;
    this.tsfnDescriptorResponse = tsfnDescriptorResponse;
    this.tsfnSubscribe = tsfnSubscribe;
    this.tsfnUnsubscribe = tsfnUnsubscribe;
    this.tsfnNotifySent = tsfnNotifySent;
  }

  @Override
  public void
  onConnectionStateChange(BluetoothDevice device, int status, int newState) {
    nativeOnConnectionStateChange(nativePointer, device, status, newState);
  }

  @Override
  public void
  onServiceAdded(int status, BluetoothGattService service) {
    nativeOnServiceAdded(nativePointer, status, service);
  }

  @Override
  public void
  onCharacteristicReadRequest(BluetoothDevice device, int requestId, int offset, BluetoothGattCharacteristic characteristic) {
    nativeOnCharacteristicReadRequest(nativePointer, device, requestId, offset, characteristic);
  }

  @Override
  public void
  onCharacteristicWriteRequest(BluetoothDevice device, int requestId, BluetoothGattCharacteristic characteristic, boolean preparedWrite, boolean responseNeeded, int offset, byte[] value) {
    nativeOnCharacteristicWriteRequest(nativePointer, device, requestId, characteristic, preparedWrite, responseNeeded, offset, value);
  }

  @Override
  public void
  onDescriptorWriteRequest(BluetoothDevice device, int requestId, BluetoothGattDescriptor descriptor, boolean preparedWrite, boolean responseNeeded, int offset, byte[] value) {
    nativeOnDescriptorWriteRequest(nativePointer, device, requestId, descriptor, preparedWrite, responseNeeded, offset, value);
  }

  @Override
  public void
  onNotificationSent(BluetoothDevice device, int status) {
    nativeOnNotificationSent(nativePointer, device, status);
  }

  @Override
  protected void
  finalize() {
    nativeOnFinalize(nativePointer, tsfnConnectionState, tsfnAddService, tsfnReadRequest, tsfnWriteRequest, tsfnDescriptorResponse, tsfnSubscribe, tsfnUnsubscribe, tsfnNotifySent);
  }

  private static native void
  nativeOnConnectionStateChange(long nativePointer, BluetoothDevice device, int status, int newState);

  private static native void
  nativeOnServiceAdded(long nativePointer, int status, BluetoothGattService service);

  private static native void
  nativeOnCharacteristicReadRequest(long nativePointer, BluetoothDevice device, int requestId, int offset, BluetoothGattCharacteristic characteristic);

  private static native void
  nativeOnCharacteristicWriteRequest(long nativePointer, BluetoothDevice device, int requestId, BluetoothGattCharacteristic characteristic, boolean preparedWrite, boolean responseNeeded, int offset, byte[] value);

  private static native void
  nativeOnDescriptorWriteRequest(long nativePointer, BluetoothDevice device, int requestId, BluetoothGattDescriptor descriptor, boolean preparedWrite, boolean responseNeeded, int offset, byte[] value);

  private static native void
  nativeOnNotificationSent(long nativePointer, BluetoothDevice device, int status);

  private static native void
  nativeOnFinalize(long nativePointer, long tsfnConnectionState, long tsfnAddService, long tsfnReadRequest, long tsfnWriteRequest, long tsfnDescriptorResponse, long tsfnSubscribe, long tsfnUnsubscribe, long tsfnNotifySent);
}
