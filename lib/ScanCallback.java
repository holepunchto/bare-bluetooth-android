package to.holepunch.bare.bluetooth;

import android.bluetooth.le.ScanResult;

public final class ScanCallback extends android.bluetooth.le.ScanCallback {
  private final long nativePointer;

  public ScanCallback(long nativePointer) {
    this.nativePointer = nativePointer;
  }

  @Override
  public void
  onScanResult(int callbackType, ScanResult result) {
    nativeOnScanResult(nativePointer, callbackType, result);
  }

  @Override
  public void
  onScanFailed(int errorCode) {
    nativeOnScanFailed(nativePointer, errorCode);
  }

  private static native void
  nativeOnScanResult(long nativePointer, int callbackType, ScanResult result);

  private static native void
  nativeOnScanFailed(long nativePointer, int errorCode);
}
