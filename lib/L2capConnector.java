package to.holepunch.bare.bluetooth;

import android.bluetooth.BluetoothSocket;
import java.io.IOException;
import java.util.concurrent.atomic.AtomicBoolean;

public final class L2capConnector implements Runnable {
  private static final long JOIN_TIMEOUT_MS = 1000;

  private final BluetoothSocket socket;
  private final long nativeId;
  private final int psm;
  private final AtomicBoolean completed = new AtomicBoolean(false);
  private volatile boolean cancelled = false;
  private Thread thread;

  public L2capConnector(BluetoothSocket socket, long nativeId, int psm) {
    this.socket = socket;
    this.nativeId = nativeId;
    this.psm = psm;
  }

  public synchronized void
  start() {
    if (thread != null || cancelled) return;

    thread = new Thread(this, "bare-bluetooth-l2cap-connect");
    thread.start();
  }

  public synchronized void
  cancel() {
    if (cancelled) return;
    cancelled = true;

    closeSocket(socket);

    Thread t = thread;

    if (t != null && t != Thread.currentThread()) {
      try {
        t.join(JOIN_TIMEOUT_MS);
      } catch (InterruptedException e) {
        Thread.currentThread().interrupt();
      }
    }
  }

  @Override
  public void
  run() {
    boolean success = false;
    String error = null;

    try {
      socket.connect();

      if (cancelled) {
        closeSocket(socket);
        error = "L2CAP connect cancelled";
      } else {
        success = true;
      }
    } catch (IOException | RuntimeException e) {
      error = cancelled ? "L2CAP connect cancelled" : errorMessage("L2CAP connect failed", e);
    }

    if (completed.compareAndSet(false, true)) {
      nativeOnComplete(nativeId, psm, success, error == null ? "" : error);
    }
  }

  private static native void
  nativeOnComplete(long nativeId, int psm, boolean success, String error);

  private static void
  closeSocket(BluetoothSocket socket) {
    try {
      socket.close();
    } catch (IOException | RuntimeException e) {
      // Ignore close errors while cancelling to avoid masking cancellation.
    }
  }

  private static String
  errorMessage(String prefix, Throwable error) {
    String message = error.getMessage();
    return message == null || message.length() == 0
      ? prefix + ": " + error.getClass().getSimpleName()
      : prefix + ": " + message;
  }
}
