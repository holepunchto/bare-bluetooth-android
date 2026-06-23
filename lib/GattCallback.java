package to.holepunch.bare.bluetooth;

import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothProfile;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

public final class GattCallback extends android.bluetooth.BluetoothGattCallback {
  private long nativePointer;
  private final Map<String, BluetoothGatt> connectedGatts = new ConcurrentHashMap<>();

  public GattCallback(long nativePointer) {
    this.nativePointer = nativePointer;
  }

  public synchronized void
  clearNativePointer() {
    this.nativePointer = 0;
  }

  @Override
  public synchronized void
  onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
    if (nativePointer == 0) return;

    if (gatt != null && gatt.getDevice() != null) {
      String address = gatt.getDevice().getAddress();

      if (status == BluetoothGatt.GATT_SUCCESS && newState == BluetoothProfile.STATE_CONNECTED) {
        connectedGatts.put(address, gatt);
      } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
        connectedGatts.remove(address);
      }
    }

    nativeOnConnectionStateChange(nativePointer, gatt, status, newState);
  }

  public BluetoothGatt
  takeConnectedGatt(String address) {
    return connectedGatts.remove(address);
  }

  @Override
  public synchronized void
  onServicesDiscovered(BluetoothGatt gatt, int status) {
    if (nativePointer == 0) return;
    nativeOnServicesDiscovered(nativePointer, gatt, status);
  }

  @Override
  public synchronized void
  onCharacteristicRead(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, byte[] value, int status) {
    if (nativePointer == 0) return;
    nativeOnCharacteristicRead(nativePointer, gatt, characteristic, value, status);
  }

  @Override
  public synchronized void
  onCharacteristicWrite(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {
    if (nativePointer == 0) return;
    nativeOnCharacteristicWrite(nativePointer, gatt, characteristic, status);
  }

  @Override
  public synchronized void
  onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, byte[] value) {
    if (nativePointer == 0) return;
    nativeOnCharacteristicChanged(nativePointer, gatt, characteristic, value);
  }

  @Override
  public synchronized void
  onDescriptorWrite(BluetoothGatt gatt, BluetoothGattDescriptor descriptor, int status) {
    if (nativePointer == 0) return;
    nativeOnDescriptorWrite(nativePointer, gatt, descriptor, status);
  }

  @Override
  public synchronized void
  onMtuChanged(BluetoothGatt gatt, int mtu, int status) {
    if (nativePointer == 0) return;
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
