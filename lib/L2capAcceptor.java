package to.holepunch.bare.bluetooth;

import android.bluetooth.BluetoothServerSocket;
import android.bluetooth.BluetoothSocket;
import java.io.IOException;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;

public final class L2capAcceptor implements Runnable {
  private final BluetoothServerSocket serverSocket;
  private final long nativeId;
  private final int psm;
  private final AtomicInteger nextSocketId = new AtomicInteger(1);
  private final Map<Integer, BluetoothSocket> acceptedSockets = new ConcurrentHashMap<>();
  private volatile boolean stopped = false;
  private Thread thread;

  public L2capAcceptor(BluetoothServerSocket serverSocket, long nativeId, int psm) {
    this.serverSocket = serverSocket;
    this.nativeId = nativeId;
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
      nativeOnError(nativeId, psm, ErrorHelper.formatMessage("L2CAP accept close failed", e));
    }

    ThreadHelper.join(thread);

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

        nativeOnAccepted(nativeId, psm, id);
      } catch (IOException | RuntimeException e) {
        if (!stopped) {
          nativeOnError(nativeId, psm, ErrorHelper.formatMessage("L2CAP accept failed", e));
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
      nativeOnError(nativeId, psm, ErrorHelper.formatMessage("Accepted L2CAP socket close failed", e));
    }
  }

  private static native void
  nativeOnAccepted(long nativeId, int psm, int socketId);

  private static native void
  nativeOnError(long nativeId, int psm, String error);
}
