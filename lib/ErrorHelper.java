package to.holepunch.bare.bluetooth;

final class ErrorHelper {
  static String
  formatMessage(String prefix, Throwable error) {
    String message = error.getMessage();
    return message == null || message.length() == 0
      ? prefix + ": " + error.getClass().getSimpleName()
      : prefix + ": " + message;
  }
}
