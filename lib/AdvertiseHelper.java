package to.holepunch.bare.bluetooth;

import android.bluetooth.le.AdvertiseData;
import android.bluetooth.le.AdvertiseSettings;
import android.bluetooth.le.BluetoothLeAdvertiser;
import android.os.ParcelUuid;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

public final class AdvertiseHelper {
  private final int advertiseMode;
  private final boolean connectable;
  private final boolean includeDeviceName;
  private final List<UUID> serviceUuids = new ArrayList<>();

  public AdvertiseHelper(int advertiseMode, boolean connectable, boolean includeDeviceName) {
    this.advertiseMode = advertiseMode;
    this.connectable = connectable;
    this.includeDeviceName = includeDeviceName;
  }

  public void
  addServiceUuid(UUID uuid) {
    serviceUuids.add(uuid);
  }

  public void
  startAdvertising(BluetoothLeAdvertiser advertiser, android.bluetooth.le.AdvertiseCallback callback) {
    AdvertiseSettings settings = new AdvertiseSettings.Builder()
      .setAdvertiseMode(advertiseMode)
      .setConnectable(connectable)
      .build();

    AdvertiseData.Builder dataBuilder = new AdvertiseData.Builder()
      .setIncludeDeviceName(includeDeviceName);

    for (UUID uuid : serviceUuids) {
      dataBuilder.addServiceUuid(new ParcelUuid(uuid));
    }

    advertiser.startAdvertising(settings, dataBuilder.build(), callback);
  }
}
