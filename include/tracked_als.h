#pragma once

#include <stdint.h>

class TrackedAls {
 public:
  bool begin(uint8_t addr = 0x23);
  bool ok() const { return _ok; }
  bool poll();
  float lux() const { return _lux; }

 private:
  bool _ok = false;
  uint8_t _addr = 0x23;
  float _lux = -1.0f;
  bool probe(uint8_t addr);
  bool write_cmd(uint8_t cmd) const;
};
