package to.holepunch.bare.bluetooth;

import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import java.util.UUID;

public final class GattServiceHelper {
  private static final UUID CCCD_UUID = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb");
  private static final byte[] ENABLE_NOTIFICATION = {0x01, 0x00};
  private static final byte[] DISABLE_NOTIFICATION = {0x00, 0x00};

  public static BluetoothGattService
  createService(UUID uuid, boolean isPrimary) {
    return new BluetoothGattService(
      uuid,
      isPrimary ? BluetoothGattService.SERVICE_TYPE_PRIMARY : BluetoothGattService.SERVICE_TYPE_SECONDARY
    );
  }

  public static BluetoothGattCharacteristic
  createCharacteristic(UUID uuid, int properties, int jsPermissions) {
    int androidPermissions = 0;

    if ((jsPermissions & 0x01) != 0) androidPermissions |= 0x01;
    if ((jsPermissions & 0x02) != 0) androidPermissions |= 0x10;
    if ((jsPermissions & 0x04) != 0) androidPermissions |= 0x02;
    if ((jsPermissions & 0x08) != 0) androidPermissions |= 0x20;

    BluetoothGattCharacteristic characteristic = new BluetoothGattCharacteristic(
      uuid, properties, androidPermissions
    );

    if ((properties & 0x30) != 0) {
      characteristic.addDescriptor(new BluetoothGattDescriptor(CCCD_UUID, 0x11));
    }

    return characteristic;
  }

  public static boolean
  subscribe(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic) {
    boolean ok = gatt.setCharacteristicNotification(characteristic, true);
    BluetoothGattDescriptor descriptor = characteristic.getDescriptor(CCCD_UUID);

    if (descriptor == null) return false;

    descriptor.setValue(ENABLE_NOTIFICATION);
    return gatt.writeDescriptor(descriptor) && ok;
  }

  public static boolean
  unsubscribe(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic) {
    boolean ok = gatt.setCharacteristicNotification(characteristic, false);
    BluetoothGattDescriptor descriptor = characteristic.getDescriptor(CCCD_UUID);

    if (descriptor == null) return false;

    descriptor.setValue(DISABLE_NOTIFICATION);
    return gatt.writeDescriptor(descriptor) && ok;
  }
}
