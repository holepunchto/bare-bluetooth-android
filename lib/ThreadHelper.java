package to.holepunch.bare.bluetooth;

final class ThreadHelper {
  private static final long JOIN_TIMEOUT_MS = 1000;

  static void
  join(Thread thread) {
    if (thread != null && thread != Thread.currentThread()) {
      try {
        thread.join(JOIN_TIMEOUT_MS);
      } catch (InterruptedException e) {
        Thread.currentThread().interrupt();
      }
    }
  }
}
