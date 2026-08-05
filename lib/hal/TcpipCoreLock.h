#pragma once

// Not available in the simulator, which has no lwIP: the guard below then compiles away.
#if __has_include(<lwip/priv/tcpip_priv.h>)
#include <lwip/priv/tcpip_priv.h>
#define CROSSINK_HAS_TCPIP_CORE_LOCK LWIP_TCPIP_CORE_LOCKING
#else
#define CROSSINK_HAS_TCPIP_CORE_LOCK 0
#endif

/**
 * Holds the lwIP core lock for a scope, unless this thread already holds it.
 *
 * The sntp_* family reaches straight into lwIP's core -- sntp_init() resolves its server
 * through dns_gethostbyname(), which allocates a UDP socket. lwIP requires the caller to
 * hold the core lock for that when it is not the TCP/IP thread, and the stack this
 * firmware builds against asserts on an unlocked call rather than tolerating it.
 *
 * The lock is a plain, non-recursive mutex, so taking it while this thread is already the
 * holder deadlocks. That happens whenever the caller is itself running in the TCP/IP
 * context, which is why the acquisition is conditional. Arduino's own configTzTime()
 * guards identically (cores/esp32/esp32-hal-time.c); this is that guard, made scope-safe.
 *
 * Never hold one across a wait: the whole network stack stalls until it is released.
 */
class TcpipCoreLock {
 public:
  TcpipCoreLock() {
#if CROSSINK_HAS_TCPIP_CORE_LOCK
    if (!sys_thread_tcpip(LWIP_CORE_LOCK_QUERY_HOLDER)) {
      LOCK_TCPIP_CORE();
      _acquired = true;
    }
#endif
  }

  ~TcpipCoreLock() {
#if CROSSINK_HAS_TCPIP_CORE_LOCK
    if (_acquired) {
      UNLOCK_TCPIP_CORE();
    }
#endif
  }

  TcpipCoreLock(const TcpipCoreLock&) = delete;
  TcpipCoreLock& operator=(const TcpipCoreLock&) = delete;

 private:
#if CROSSINK_HAS_TCPIP_CORE_LOCK
  bool _acquired = false;
#endif
};
