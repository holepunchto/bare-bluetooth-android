package to.holepunch.bare.bluetooth;

import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattServer;
import android.bluetooth.BluetoothGattService;

public final class GattServerCallback extends android.bluetooth.BluetoothGattServerCallback {
  private final long nativeId;

  public GattServerCallback(long nativeId) {
    this.nativeId = nativeId;
  }

  @Override
  public void
  onConnectionStateChange(BluetoothDevice device, int status, int newState) {
    nativeOnConnectionStateChange(nativeId, device, status, newState);
  }

  @Override
  public void
  onServiceAdded(int status, BluetoothGattService service) {
    nativeOnServiceAdded(nativeId, status, service);
  }

  @Override
  public void
  onCharacteristicReadRequest(BluetoothDevice device, int requestId, int offset, BluetoothGattCharacteristic characteristic) {
    nativeOnCharacteristicReadRequest(nativeId, device, requestId, offset, characteristic);
  }

  @Override
  public void
  onCharacteristicWriteRequest(BluetoothDevice device, int requestId, BluetoothGattCharacteristic characteristic, boolean preparedWrite, boolean responseNeeded, int offset, byte[] value) {
    nativeOnCharacteristicWriteRequest(nativeId, device, requestId, characteristic, preparedWrite, responseNeeded, offset, value);
  }

  @Override
  public void
  onDescriptorWriteRequest(BluetoothDevice device, int requestId, BluetoothGattDescriptor descriptor, boolean preparedWrite, boolean responseNeeded, int offset, byte[] value) {
    nativeOnDescriptorWriteRequest(nativeId, device, requestId, descriptor, preparedWrite, responseNeeded, offset, value);
  }

  @Override
  public void
  onNotificationSent(BluetoothDevice device, int status) {
    nativeOnNotificationSent(nativeId, device, status);
  }

  private static native void
  nativeOnConnectionStateChange(long nativeId, BluetoothDevice device, int status, int newState);

  private static native void
  nativeOnServiceAdded(long nativeId, int status, BluetoothGattService service);

  private static native void
  nativeOnCharacteristicReadRequest(long nativeId, BluetoothDevice device, int requestId, int offset, BluetoothGattCharacteristic characteristic);

  private static native void
  nativeOnCharacteristicWriteRequest(long nativeId, BluetoothDevice device, int requestId, BluetoothGattCharacteristic characteristic, boolean preparedWrite, boolean responseNeeded, int offset, byte[] value);

  private static native void
  nativeOnDescriptorWriteRequest(long nativeId, BluetoothDevice device, int requestId, BluetoothGattDescriptor descriptor, boolean preparedWrite, boolean responseNeeded, int offset, byte[] value);

  private static native void
  nativeOnNotificationSent(long nativeId, BluetoothDevice device, int status);
}
