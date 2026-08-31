package to.holepunch.bare.bluetooth;

import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothProfile;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;

public final class GattCallback extends android.bluetooth.BluetoothGattCallback {
  private static final UUID CCCD_UUID = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb");
  private static final byte[] ENABLE_NOTIFICATION = {0x01, 0x00};
  private static final byte[] DISABLE_NOTIFICATION = {0x00, 0x00};

  private final long nativeId;
  private final Map<String, BluetoothGatt> connectedGatts = new ConcurrentHashMap<>();
  private final GattQueue queue = new GattQueue();

  private long peripheralId;

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
        queue.clear();
      }
    }

    nativeOnConnectionStateChange(nativeId, gatt, status, newState);
  }

  public BluetoothGatt
  takeConnectedGatt(String address) {
    return connectedGatts.remove(address);
  }

  public void
  setPeripheralId(long peripheralId) {
    this.peripheralId = peripheralId;
  }

  // a locally initiated close() unregisters this callback, so no further
  // completion or state-change callback will ever arrive to release the queue
  public void
  clearQueue() {
    queue.clear();
  }

  public boolean
  discoverServices(BluetoothGatt gatt) {
    queue.enqueue(GattQueue.Kind.SERVICES_DISCOVERED, new GattQueue.Op() {
      @Override
      public boolean
      run() {
        return gatt.discoverServices();
      }

      @Override
      public void
      fail() {
        nativeOnServicesDiscovered(peripheralId, gatt, BluetoothGatt.GATT_FAILURE);
      }
    });
    return true;
  }

  public boolean
  read(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic) {
    queue.enqueue(GattQueue.Kind.CHARACTERISTIC_READ, new GattQueue.Op() {
      @Override
      public boolean
      run() {
        return gatt.readCharacteristic(characteristic);
      }

      @Override
      public void
      fail() {
        nativeOnCharacteristicRead(peripheralId, gatt, characteristic, null, BluetoothGatt.GATT_FAILURE);
      }
    });
    return true;
  }

  public boolean
  write(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, byte[] value, boolean withResponse) {
    queue.enqueue(GattQueue.Kind.CHARACTERISTIC_WRITE, new GattQueue.Op() {
      @Override
      public boolean
      run() {
        characteristic.setWriteType(
          withResponse ? BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT : BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE
        );
        characteristic.setValue(value);
        return gatt.writeCharacteristic(characteristic);
      }

      @Override
      public void
      fail() {
        nativeOnCharacteristicWrite(peripheralId, gatt, characteristic, BluetoothGatt.GATT_FAILURE);
      }
    });
    return true;
  }

  public boolean
  setNotify(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, boolean enable) {
    BluetoothGattDescriptor descriptor = characteristic.getDescriptor(CCCD_UUID);

    if (descriptor == null) {
      // no CCCD to write: still toggle local routing, as before
      gatt.setCharacteristicNotification(characteristic, enable);
      return false;
    }

    queue.enqueue(GattQueue.Kind.DESCRIPTOR_WRITE, new GattQueue.Op() {
      @Override
      public boolean
      run() {
        // returning false must mean nothing was issued — the queue reports
        // failure and moves on — so don't write the descriptor once
        // setCharacteristicNotification has already failed
        boolean ok = gatt.setCharacteristicNotification(characteristic, enable);
        descriptor.setValue(enable ? ENABLE_NOTIFICATION : DISABLE_NOTIFICATION);
        return ok && gatt.writeDescriptor(descriptor);
      }

      @Override
      public void
      fail() {
        nativeOnDescriptorWrite(peripheralId, gatt, descriptor, BluetoothGatt.GATT_FAILURE);
      }
    });
    return true;
  }

  public boolean
  requestMtu(BluetoothGatt gatt, int mtu) {
    queue.enqueue(GattQueue.Kind.MTU_CHANGED, new GattQueue.Op() {
      @Override
      public boolean
      run() {
        return gatt.requestMtu(mtu);
      }

      @Override
      public void
      fail() {
        nativeOnMtuChanged(peripheralId, gatt, mtu, BluetoothGatt.GATT_FAILURE);
      }
    });
    return true;
  }

  @Override
  public void
  onServicesDiscovered(BluetoothGatt gatt, int status) {
    nativeOnServicesDiscovered(peripheralId, gatt, status);
    queue.completed(GattQueue.Kind.SERVICES_DISCOVERED);
  }

  @Override
  public void
  onCharacteristicRead(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, byte[] value, int status) {
    nativeOnCharacteristicRead(peripheralId, gatt, characteristic, value, status);
    queue.completed(GattQueue.Kind.CHARACTERISTIC_READ);
  }

  // API < 33 invokes only this legacy signature; 33+ invokes only the byte[]
  // variant above, so there is no double dispatch
  @Override
  public void
  onCharacteristicRead(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {
    onCharacteristicRead(gatt, characteristic, characteristic.getValue(), status);
  }

  @Override
  public void
  onCharacteristicWrite(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {
    nativeOnCharacteristicWrite(peripheralId, gatt, characteristic, status);
    queue.completed(GattQueue.Kind.CHARACTERISTIC_WRITE);
  }

  @Override
  public void
  onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, byte[] value) {
    nativeOnCharacteristicChanged(peripheralId, gatt, characteristic, value);
  }

  @Override
  public void
  onDescriptorWrite(BluetoothGatt gatt, BluetoothGattDescriptor descriptor, int status) {
    nativeOnDescriptorWrite(peripheralId, gatt, descriptor, status);
    queue.completed(GattQueue.Kind.DESCRIPTOR_WRITE);
  }

  @Override
  public void
  onMtuChanged(BluetoothGatt gatt, int mtu, int status) {
    nativeOnMtuChanged(peripheralId, gatt, mtu, status);
    queue.completed(GattQueue.Kind.MTU_CHANGED);
  }

  private static native void
  nativeOnConnectionStateChange(long nativeId, BluetoothGatt gatt, int status, int newState);

  private static native void
  nativeOnServicesDiscovered(long peripheralId, BluetoothGatt gatt, int status);

  private static native void
  nativeOnCharacteristicRead(long peripheralId, BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, byte[] value, int status);

  private static native void
  nativeOnCharacteristicWrite(long peripheralId, BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status);

  private static native void
  nativeOnCharacteristicChanged(long peripheralId, BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, byte[] value);

  private static native void
  nativeOnDescriptorWrite(long peripheralId, BluetoothGatt gatt, BluetoothGattDescriptor descriptor, int status);

  private static native void
  nativeOnMtuChanged(long peripheralId, BluetoothGatt gatt, int mtu, int status);
}
