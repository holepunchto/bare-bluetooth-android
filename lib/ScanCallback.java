package to.holepunch.bare.bluetooth;

import android.bluetooth.le.ScanResult;

public final class ScanCallback extends android.bluetooth.le.ScanCallback {
  private final long nativePointer;
  private final long tsfnDiscover;
  private final long tsfnScanFail;

  public ScanCallback(long nativePointer, long tsfnDiscover, long tsfnScanFail) {
    this.nativePointer = nativePointer;
    this.tsfnDiscover = tsfnDiscover;
    this.tsfnScanFail = tsfnScanFail;
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

  @Override
  protected void
  finalize() {
    nativeOnFinalize(nativePointer, tsfnDiscover, tsfnScanFail);
  }

  private static native void
  nativeOnScanResult(long nativePointer, int callbackType, ScanResult result);

  private static native void
  nativeOnScanFailed(long nativePointer, int errorCode);

  private static native void
  nativeOnFinalize(long nativePointer, long tsfnDiscover, long tsfnScanFail);
}
