package to.holepunch.bare.bluetooth;

import android.bluetooth.le.ScanResult;

public final class ScanCallback extends android.bluetooth.le.ScanCallback {
  private long nativePointer;

  public ScanCallback(long nativePointer) {
    this.nativePointer = nativePointer;
  }

  public synchronized void
  clearNativePointer() {
    this.nativePointer = 0;
  }

  @Override
  public synchronized void
  onScanResult(int callbackType, ScanResult result) {
    if (nativePointer == 0) return;
    nativeOnScanResult(nativePointer, callbackType, result);
  }

  @Override
  public synchronized void
  onScanFailed(int errorCode) {
    if (nativePointer == 0) return;
    nativeOnScanFailed(nativePointer, errorCode);
  }

  private static native void
  nativeOnScanResult(long nativePointer, int callbackType, ScanResult result);

  private static native void
  nativeOnScanFailed(long nativePointer, int errorCode);
}
