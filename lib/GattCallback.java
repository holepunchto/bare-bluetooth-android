package to.holepunch.bare.bluetooth;

import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothProfile;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

public final class GattCallback extends android.bluetooth.BluetoothGattCallback {
  private final long nativeId;
  private final Map<String, BluetoothGatt> connectedGatts = new ConcurrentHashMap<>();

  public GattCallback(long nativeId) {
    this.nativeId = nativeId;
  }

  @Override
  public void
  onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
    if (gatt != null && gatt.getDevice() != null) {
      String address = gatt.getDevice().getAddress();

      if (status == BluetoothGatt.GATT_SUCCESS && newState == BluetoothProfile.STATE_CONNECTED) {
        connectedGatts.put(address, gatt);
      } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
        connectedGatts.remove(address);
      }
    }

    nativeOnConnectionStateChange(nativeId, gatt, status, newState);
  }

  public BluetoothGatt
  takeConnectedGatt(String address) {
    return connectedGatts.remove(address);
  }

  @Override
  public void
  onServicesDiscovered(BluetoothGatt gatt, int status) {
    nativeOnServicesDiscovered(nativeId, gatt, status);
  }

  @Override
  public void
  onCharacteristicRead(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, byte[] value, int status) {
    nativeOnCharacteristicRead(nativeId, gatt, characteristic, value, status);
  }

  @Override
  public void
  onCharacteristicWrite(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {
    nativeOnCharacteristicWrite(nativeId, gatt, characteristic, status);
  }

  @Override
  public void
  onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, byte[] value) {
    nativeOnCharacteristicChanged(nativeId, gatt, characteristic, value);
  }

  @Override
  public void
  onDescriptorWrite(BluetoothGatt gatt, BluetoothGattDescriptor descriptor, int status) {
    nativeOnDescriptorWrite(nativeId, gatt, descriptor, status);
  }

  @Override
  public void
  onMtuChanged(BluetoothGatt gatt, int mtu, int status) {
    nativeOnMtuChanged(nativeId, gatt, mtu, status);
  }

  private static native void
  nativeOnConnectionStateChange(long nativeId, BluetoothGatt gatt, int status, int newState);

  private static native void
  nativeOnServicesDiscovered(long nativeId, BluetoothGatt gatt, int status);

  private static native void
  nativeOnCharacteristicRead(long nativeId, BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, byte[] value, int status);

  private static native void
  nativeOnCharacteristicWrite(long nativeId, BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status);

  private static native void
  nativeOnCharacteristicChanged(long nativeId, BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, byte[] value);

  private static native void
  nativeOnDescriptorWrite(long nativeId, BluetoothGatt gatt, BluetoothGattDescriptor descriptor, int status);

  private static native void
  nativeOnMtuChanged(long nativeId, BluetoothGatt gatt, int mtu, int status);
}
