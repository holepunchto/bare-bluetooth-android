package to.holepunch.bare.bluetooth;

import android.bluetooth.BluetoothSocket;
import java.io.IOException;
import java.io.InputStream;
import java.util.Arrays;
import java.util.concurrent.atomic.AtomicBoolean;

public final class L2capReader implements Runnable {
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
    if (thread != null) return;

    thread = new Thread(this, "bare-bluetooth-l2cap-read");
    thread.start();
  }

  public void
  stop() {
    stopped = true;
    closeSocket();
    joinThread();
  }

  @Override
  public void
  run() {
    try {
      InputStream input = socket.getInputStream();
      nativeOnOpen(nativePointer);

      byte[] buffer = new byte[4096];

      while (!stopped) {
        int bytesRead = input.read(buffer);

        if (bytesRead == -1) {
          nativeOnEnd(nativePointer);
          break;
        }

        if (bytesRead > 0) {
          nativeOnData(nativePointer, Arrays.copyOf(buffer, bytesRead));
        }
      }
    } catch (IOException e) {
      if (!stopped) nativeOnError(nativePointer, "Read error");
    } catch (RuntimeException e) {
      if (!stopped) nativeOnError(nativePointer, "Read error");
    } finally {
      closeSocket();
      if (closed.compareAndSet(false, true)) nativeOnClose(nativePointer);
    }
  }

  private void
  closeSocket() {
    try {
      socket.close();
    } catch (IOException e) {
      // Ignore close errors during teardown.
    } catch (RuntimeException e) {
      // Ignore close errors during teardown.
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
