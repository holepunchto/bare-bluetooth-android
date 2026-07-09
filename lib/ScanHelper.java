package to.holepunch.bare.bluetooth;

import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanFilter;
import android.bluetooth.le.ScanSettings;
import android.os.ParcelUuid;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

public final class ScanHelper {
  private final int scanMode;
  private final List<UUID> uuids = new ArrayList<>();

  public ScanHelper(int scanMode) {
    this.scanMode = scanMode;
  }

  public void
  addServiceUuid(UUID uuid) {
    uuids.add(uuid);
  }

  public void
  startScan(BluetoothLeScanner scanner, android.bluetooth.le.ScanCallback callback) {
    ScanSettings settings = new ScanSettings.Builder()
      .setScanMode(scanMode)
      .build();

    List<ScanFilter> filters = null;

    if (!uuids.isEmpty()) {
      filters = new ArrayList<>();

      for (UUID uuid : uuids) {
        filters.add(
          new ScanFilter.Builder()
            .setServiceUuid(new ParcelUuid(uuid))
            .build()
        );
      }
    }

    scanner.startScan(filters, settings, callback);
  }
}
