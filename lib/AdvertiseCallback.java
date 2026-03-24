package to.holepunch.bare.bluetooth;

import android.bluetooth.le.AdvertiseSettings;

public final class AdvertiseCallback extends android.bluetooth.le.AdvertiseCallback {
  private final long nativePointer;

  public AdvertiseCallback(long nativePointer) {
    this.nativePointer = nativePointer;
  }

  @Override
  public void
  onStartSuccess(AdvertiseSettings settingsInEffect) {
    nativeOnStartSuccess(nativePointer, settingsInEffect);
  }

  @Override
  public void
  onStartFailure(int errorCode) {
    nativeOnStartFailure(nativePointer, errorCode);
  }

  private static native void
  nativeOnStartSuccess(long nativePointer, AdvertiseSettings settingsInEffect);

  private static native void
  nativeOnStartFailure(long nativePointer, int errorCode);
}
