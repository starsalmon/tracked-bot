#pragma once

#include <stdint.h>

/** Nose VL53L0X on I2C (same bus as OLED). Optional until the breakout is fitted. */
class TrackedTof {
 public:
  bool begin();
  void poll();
  bool ok() const { return _ok; }
  bool present() const { return _present; }
  float range_m() const { return _range_m; }

 private:
  bool _present = false;
  bool _ok = false;
  float _range_m = -1.0f;
};
