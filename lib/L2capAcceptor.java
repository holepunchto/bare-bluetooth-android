package to.holepunch.bare.bluetooth;

import android.bluetooth.BluetoothServerSocket;
import android.bluetooth.BluetoothSocket;
import java.io.IOException;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;

public final class L2capAcceptor implements Runnable {
  private final BluetoothServerSocket serverSocket;
  private final long nativePointer;
  private final int psm;
  private final AtomicInteger nextSocketId = new AtomicInteger(1);
  private final Map<Integer, BluetoothSocket> acceptedSockets = new ConcurrentHashMap<>();
  private volatile boolean stopped = false;
  private Thread thread;

  public L2capAcceptor(BluetoothServerSocket serverSocket, long nativePointer, int psm) {
    this.serverSocket = serverSocket;
    this.nativePointer = nativePointer;
    this.psm = psm;
  }

  public synchronized void
  start() {
    if (thread != null) return;

    thread = new Thread(this, "bare-bluetooth-l2cap-accept");
    thread.start();
  }

  public void
  stop() {
    stopped = true;
    closeServerSocket();
    joinThread();
  }

  public BluetoothSocket
  takeSocket(int id) {
    return acceptedSockets.remove(id);
  }

  @Override
  public void
  run() {
    while (!stopped) {
      try {
        BluetoothSocket socket = serverSocket.accept();
        if (socket == null) continue;

        if (stopped) {
          try {
            socket.close();
          } catch (IOException e) {
            // Ignore close errors during teardown.
          }
          break;
        }

        int id = nextSocketId.getAndIncrement();
        acceptedSockets.put(id, socket);
        nativeOnAccepted(nativePointer, psm, id);
      } catch (IOException e) {
        if (!stopped) nativeOnError(nativePointer, psm, "L2CAP accept failed");
        break;
      } catch (RuntimeException e) {
        if (!stopped) nativeOnError(nativePointer, psm, "L2CAP accept failed");
        break;
      }
    }
  }

  private void
  closeServerSocket() {
    try {
      serverSocket.close();
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
  nativeOnAccepted(long nativePointer, int psm, int socketId);

  private static native void
  nativeOnError(long nativePointer, int psm, String error);
}
