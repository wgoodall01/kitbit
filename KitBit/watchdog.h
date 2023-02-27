#pragma once

class Watchdog {
  public:
    bool enable(int timeout_ms);
    void reset();
};

// The global singleton instance.
extern class Watchdog Watchdog;
