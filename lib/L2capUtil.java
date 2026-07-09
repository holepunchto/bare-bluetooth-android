package to.holepunch.bare.bluetooth;

final class L2capUtil {
  private static final long JOIN_TIMEOUT_MS = 1000;

  static String
  errorMessage(String prefix, Throwable error) {
    String message = error.getMessage();
    return message == null || message.length() == 0
      ? prefix + ": " + error.getClass().getSimpleName()
      : prefix + ": " + message;
  }

  static void
  joinThread(Thread thread) {
    if (thread != null && thread != Thread.currentThread()) {
      try {
        thread.join(JOIN_TIMEOUT_MS);
      } catch (InterruptedException e) {
        Thread.currentThread().interrupt();
      }
    }
  }
}
