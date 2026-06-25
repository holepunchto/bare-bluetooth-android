package to.holepunch.bare.bluetooth;

import android.bluetooth.le.AdvertiseSettings;

public final class AdvertiseCallback extends android.bluetooth.le.AdvertiseCallback {
  private final long nativeId;

  public AdvertiseCallback(long nativeId) {
    this.nativeId = nativeId;
  }

  @Override
  public void
  onStartSuccess(AdvertiseSettings settingsInEffect) {
    nativeOnStartSuccess(nativeId, settingsInEffect);
  }

  @Override
  public void
  onStartFailure(int errorCode) {
    nativeOnStartFailure(nativeId, errorCode);
  }

  private static native void
  nativeOnStartSuccess(long nativeId, AdvertiseSettings settingsInEffect);

  private static native void
  nativeOnStartFailure(long nativeId, int errorCode);
}
