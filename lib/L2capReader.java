package to.holepunch.bare.bluetooth;

import android.bluetooth.BluetoothSocket;
import java.io.IOException;
import java.io.InputStream;
import java.util.Arrays;
import java.util.concurrent.atomic.AtomicBoolean;

public final class L2capReader implements Runnable {
  private final BluetoothSocket socket;
  private final long nativeId;
  private final AtomicBoolean closed = new AtomicBoolean(false);
  private volatile boolean stopped = false;
  private Thread thread;

  public L2capReader(BluetoothSocket socket, long nativeId) {
    this.socket = socket;
    this.nativeId = nativeId;
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

    L2capUtil.joinThread(thread);
  }

  @Override
  public void
  run() {
    try {
      InputStream input = socket.getInputStream();

      if (stopped) return;

      nativeOnOpen(nativeId);

      byte[] buffer = new byte[4096];

      while (!stopped) {
        int bytesRead = input.read(buffer);

        if (bytesRead == -1) {
          if (!stopped) nativeOnEnd(nativeId);
          break;
        }

        if (bytesRead > 0) {
          if (stopped) break;
          nativeOnData(nativeId, Arrays.copyOf(buffer, bytesRead));
        }
      }
    } catch (IOException | RuntimeException e) {
      if (!stopped) nativeOnError(nativeId, L2capUtil.errorMessage("Read error", e));
    } finally {
      closeSocket();
      if (closed.compareAndSet(false, true)) nativeOnClose(nativeId);
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

  private static native void
  nativeOnOpen(long nativeId);

  private static native void
  nativeOnData(long nativeId, byte[] data);

  private static native void
  nativeOnEnd(long nativeId);

  private static native void
  nativeOnError(long nativeId, String message);

  private static native void
  nativeOnClose(long nativeId);
}
