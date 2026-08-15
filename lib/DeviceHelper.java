package to.holepunch.bare.bluetooth;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import java.util.Set;

final class DeviceHelper {
  // Returns the bonded devices as "address\tname" records separated by
  // newlines. A single delimited String keeps the JNI surface narrow, matching
  // how the other helpers here avoid marshalling arrays.
  //
  // Classic-only devices are skipped: they have no GATT server, so they are
  // never something a central can connect to.
  static String
  bondedDevices(BluetoothAdapter adapter) {
    Set<BluetoothDevice> devices = adapter.getBondedDevices();

    if (devices == null) return "";

    StringBuilder out = new StringBuilder();

    for (BluetoothDevice device : devices) {
      int type = device.getType();

      if (type != BluetoothDevice.DEVICE_TYPE_LE && type != BluetoothDevice.DEVICE_TYPE_DUAL) {
        continue;
      }

      // Requires BLUETOOTH_CONNECT on API 31+; null when unavailable.
      String name = device.getName();

      if (out.length() > 0) out.append('\n');

      out.append(device.getAddress()).append('\t').append(name == null ? "" : name);
    }

    return out.toString();
  }
}
