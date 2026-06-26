package to.holepunch.bare.bluetooth;

import android.bluetooth.le.ScanResult;

public final class ScanCallback extends android.bluetooth.le.ScanCallback {
  private final long nativeId;

  public ScanCallback(long nativeId) {
    this.nativeId = nativeId;
  }

  @Override
  public void
  onScanResult(int callbackType, ScanResult result) {
    nativeOnScanResult(nativeId, callbackType, result);
  }

  @Override
  public void
  onScanFailed(int errorCode) {
    nativeOnScanFailed(nativeId, errorCode);
  }

  private static native void
  nativeOnScanResult(long nativeId, int callbackType, ScanResult result);

  private static native void
  nativeOnScanFailed(long nativeId, int errorCode);
}
