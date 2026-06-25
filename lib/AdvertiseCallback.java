package to.holepunch.bare.bluetooth;

import android.bluetooth.le.AdvertiseSettings;

public final class AdvertiseCallback extends android.bluetooth.le.AdvertiseCallback {
  private final long nativePointer;

  private long tsfnAdvertiseError;

  public AdvertiseCallback(long nativePointer) {
    this.nativePointer = nativePointer;
  }

  public void
  setTsfn(long tsfnAdvertiseError) {
    this.tsfnAdvertiseError = tsfnAdvertiseError;
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

  @Override
  protected void
  finalize() {
    nativeOnFinalize(nativePointer, tsfnAdvertiseError);
  }

  private static native void
  nativeOnStartSuccess(long nativePointer, AdvertiseSettings settingsInEffect);

  private static native void
  nativeOnStartFailure(long nativePointer, int errorCode);

  private static native void
  nativeOnFinalize(long nativePointer, long tsfnAdvertiseError);
}
