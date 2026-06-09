package to.holepunch.bare.bluetooth;

import android.bluetooth.BluetoothSocket;
import java.io.IOException;
import java.io.InputStream;
import java.util.Arrays;
import java.util.concurrent.atomic.AtomicBoolean;

public final class L2capReader implements Runnable {
  private static final long JOIN_TIMEOUT_MS = 1000;

  private final BluetoothSocket socket;
  private final long nativePointer;
  private final AtomicBoolean closed = new AtomicBoolean(false);
  private volatile boolean stopped = false;
  private Thread thread;

  public L2capReader(BluetoothSocket socket, long nativePointer) {
    this.socket = socket;
    this.nativePointer = nativePointer;
  }

  public synchronized void
  start() {
    if (thread != null || stopped) return;

    thread = new Thread(this, "bare-bluetooth-l2cap-read");
    thread.start();
  }

  public synchronized void
  stop() {
    if (stopped) return;

    stopped = true;
    closeSocket();

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
    try {
      InputStream input = socket.getInputStream();

      if (stopped) return;

      nativeOnOpen(nativePointer);

      byte[] buffer = new byte[4096];

      while (!stopped) {
        int bytesRead = input.read(buffer);

        if (bytesRead == -1) {
          if (!stopped) nativeOnEnd(nativePointer);
          break;
        }

        if (bytesRead > 0) {
          if (stopped) break;
          nativeOnData(nativePointer, Arrays.copyOf(buffer, bytesRead));
        }
      }
    } catch (IOException | RuntimeException e) {
      if (!stopped) nativeOnError(nativePointer, errorMessage("Read error", e));
    } finally {
      closeSocket();
      if (closed.compareAndSet(false, true)) nativeOnClose(nativePointer);
    }
  }

  private void
  closeSocket() {
    try {
      socket.close();
    } catch (IOException | RuntimeException e) {
      // Ignore close errors during teardown to avoid duplicate close events.
    }
  }

  private static String
  errorMessage(String prefix, Throwable error) {
    String message = error.getMessage();
    return message == null || message.length() == 0
      ? prefix + ": " + error.getClass().getSimpleName()
      : prefix + ": " + message;
  }

  private static native void
  nativeOnOpen(long nativePointer);

  private static native void
  nativeOnData(long nativePointer, byte[] data);

  private static native void
  nativeOnEnd(long nativePointer);

  private static native void
  nativeOnError(long nativePointer, String message);

  private static native void
  nativeOnClose(long nativePointer);
}
