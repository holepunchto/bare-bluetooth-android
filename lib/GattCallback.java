package to.holepunch.bare.bluetooth;

import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;

public final class GattCallback extends android.bluetooth.BluetoothGattCallback {
  private final long nativePointer;

  public GattCallback(long nativePointer) {
    this.nativePointer = nativePointer;
  }

  @Override
  public void
  onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
    nativeOnConnectionStateChange(nativePointer, gatt, status, newState);
  }

  @Override
  public void
  onServicesDiscovered(BluetoothGatt gatt, int status) {
    nativeOnServicesDiscovered(nativePointer, gatt, status);
  }

  @Override
  public void
  onCharacteristicRead(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, byte[] value, int status) {
    nativeOnCharacteristicRead(nativePointer, gatt, characteristic, value, status);
  }

  @Override
  public void
  onCharacteristicWrite(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {
    nativeOnCharacteristicWrite(nativePointer, gatt, characteristic, status);
  }

  @Override
  public void
  onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, byte[] value) {
    nativeOnCharacteristicChanged(nativePointer, gatt, characteristic, value);
  }

  @Override
  public void
  onDescriptorWrite(BluetoothGatt gatt, BluetoothGattDescriptor descriptor, int status) {
    nativeOnDescriptorWrite(nativePointer, gatt, descriptor, status);
  }

  @Override
  public void
  onMtuChanged(BluetoothGatt gatt, int mtu, int status) {
    nativeOnMtuChanged(nativePointer, gatt, mtu, status);
  }

  private static native void
  nativeOnConnectionStateChange(long nativePointer, BluetoothGatt gatt, int status, int newState);

  private static native void
  nativeOnServicesDiscovered(long nativePointer, BluetoothGatt gatt, int status);

  private static native void
  nativeOnCharacteristicRead(long nativePointer, BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, byte[] value, int status);

  private static native void
  nativeOnCharacteristicWrite(long nativePointer, BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status);

  private static native void
  nativeOnCharacteristicChanged(long nativePointer, BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, byte[] value);

  private static native void
  nativeOnDescriptorWrite(long nativePointer, BluetoothGatt gatt, BluetoothGattDescriptor descriptor, int status);

  private static native void
  nativeOnMtuChanged(long nativePointer, BluetoothGatt gatt, int mtu, int status);
}
