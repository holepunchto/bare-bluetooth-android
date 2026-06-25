package to.holepunch.bare.bluetooth;

import android.bluetooth.BluetoothServerSocket;
import android.bluetooth.BluetoothSocket;
import java.io.IOException;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;

public final class L2capAcceptor implements Runnable {
  private static final long JOIN_TIMEOUT_MS = 1000;

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
    if (thread != null || stopped) return;

    thread = new Thread(this, "bare-bluetooth-l2cap-accept");
    thread.start();
  }

  public synchronized void
  stop() {
    if (stopped) return;
    stopped = true;

    try {
      serverSocket.close();
    } catch (IOException | RuntimeException e) {
      nativeOnError(nativePointer, psm, errorMessage("L2CAP accept close failed", e));
    }

    Thread t = thread;

    if (t != null && t != Thread.currentThread()) {
      try {
        t.join(JOIN_TIMEOUT_MS);
      } catch (InterruptedException e) {
        Thread.currentThread().interrupt();
      }
    }

    for (BluetoothSocket socket : acceptedSockets.values()) {
      closeAcceptedSocket(socket);
    }

    acceptedSockets.clear();
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
          closeAcceptedSocket(socket);
          break;
        }

        int id = nextSocketId.getAndIncrement();
        acceptedSockets.put(id, socket);

        if (stopped) {
          BluetoothSocket accepted = acceptedSockets.remove(id);
          if (accepted != null) closeAcceptedSocket(accepted);
          break;
        }

        nativeOnAccepted(nativePointer, psm, id);
      } catch (IOException | RuntimeException e) {
        if (!stopped) {
          nativeOnError(nativePointer, psm, errorMessage("L2CAP accept failed", e));
        }

        break;
      }
    }
  }

  private void
  closeAcceptedSocket(BluetoothSocket socket) {
    try {
      socket.close();
    } catch (IOException | RuntimeException e) {
      nativeOnError(nativePointer, psm, errorMessage("Accepted L2CAP socket close failed", e));
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
  nativeOnAccepted(long nativePointer, int psm, int socketId);

  private static native void
  nativeOnError(long nativePointer, int psm, String error);
}
