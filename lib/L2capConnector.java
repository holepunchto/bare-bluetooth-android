package to.holepunch.bare.bluetooth;

import android.bluetooth.BluetoothSocket;
import java.io.IOException;
import java.util.concurrent.atomic.AtomicBoolean;

public final class L2capConnector implements Runnable {
  private final BluetoothSocket socket;
  private final long nativePointer;
  private final int psm;
  private final AtomicBoolean completed = new AtomicBoolean(false);
  private volatile boolean cancelled = false;
  private Thread thread;

  public L2capConnector(BluetoothSocket socket, long nativePointer, int psm) {
    this.socket = socket;
    this.nativePointer = nativePointer;
    this.psm = psm;
  }

  public synchronized void
  start() {
    if (thread != null) return;

    thread = new Thread(this, "bare-bluetooth-l2cap-connect");
    thread.start();
  }

  public void
  cancel() {
    cancelled = true;
    closeSocket();
    joinThread();
  }

  @Override
  public void
  run() {
    boolean success = false;
    String error = null;

    try {
      socket.connect();
      success = !cancelled;
      if (cancelled) error = "L2CAP connect cancelled";
    } catch (IOException e) {
      error = cancelled ? "L2CAP connect cancelled" : "L2CAP connect failed";
    } catch (RuntimeException e) {
      error = cancelled ? "L2CAP connect cancelled" : "L2CAP connect failed";
    }

    if (completed.compareAndSet(false, true)) {
      nativeOnComplete(nativePointer, psm, success, error == null ? "" : error);
    }
  }

  private void
  closeSocket() {
    try {
      socket.close();
    } catch (IOException e) {
      // Ignore close errors while cancelling.
    } catch (RuntimeException e) {
      // Ignore close errors while cancelling.
    }
  }

  private void
  joinThread() {
    Thread t = thread;
    if (t == null || t == Thread.currentThread()) return;

    try {
      t.join();
    } catch (InterruptedException e) {
      Thread.currentThread().interrupt();
    }
  }

  private static native void
  nativeOnComplete(long nativePointer, int psm, boolean success, String error);
}
