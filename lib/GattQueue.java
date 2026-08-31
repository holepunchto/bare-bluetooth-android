package to.holepunch.bare.bluetooth;

import java.util.ArrayDeque;

// BluetoothGatt allows one client operation in flight per connection: a second
// initiate returns false or clobbers the pending one. Operations queue here and
// are issued serially, advancing when the matching completion callback fires on
// the owning GattCallback.
public final class GattQueue {
  public enum Kind {
    SERVICES_DISCOVERED,
    CHARACTERISTIC_READ,
    CHARACTERISTIC_WRITE,
    DESCRIPTOR_WRITE,
    MTU_CHANGED
  }

  public interface Op {
    boolean run();

    void fail();
  }

  private static final class Pending {
    final Kind kind;
    final Op op;

    Pending(Kind kind, Op op) {
      this.kind = kind;
      this.op = op;
    }
  }

  private final ArrayDeque<Pending> pending = new ArrayDeque<>();
  private Pending inFlight = null;

  public void
  enqueue(Kind kind, Op op) {
    synchronized (this) {
      pending.add(new Pending(kind, op));
    }
    drain();
  }

  // Only the in-flight op's own completion advances the queue: an unsolicited
  // event (e.g. a peer-initiated MTU change) must not release it while the
  // radio is still busy.
  public void
  completed(Kind kind) {
    synchronized (this) {
      if (inFlight == null || inFlight.kind != kind) return;
      inFlight = null;
    }
    drain();
  }

  public void
  clear() {
    synchronized (this) {
      pending.clear();
      inFlight = null;
    }
  }

  private void
  drain() {
    while (true) {
      Pending next;
      synchronized (this) {
        if (inFlight != null) return;
        next = pending.poll();
        if (next == null) return;
        inFlight = next;
      }

      boolean ok;
      try {
        ok = next.op.run();
      } catch (Exception e) {
        // ops run on whatever thread freed the queue; an escaping exception
        // would kill a binder thread instead of reaching the caller
        ok = false;
      }
      if (ok) return;

      // report before releasing so the failure event cannot land after a
      // successor op's completion
      next.op.fail();
      synchronized (this) {
        // a concurrent completion or clear() may have released this op
        // already and moved the queue on — it owns the drain now
        if (inFlight != next) return;
        inFlight = null;
      }
    }
  }
}
