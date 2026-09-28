#include "tracked_als.h"

#include <Arduino.h>
#include <Wire.h>

#include "tracked_pins.h"

namespace {
constexpr uint8_t kPowerOn = 0x01;
constexpr uint8_t kContHres = 0x10;
}  // namespace

bool TrackedAls::write_cmd(uint8_t cmd) const {
  Wire.beginTransmission(_addr);
  Wire.write(cmd);
  return Wire.endTransmission() == 0;
}

bool TrackedAls::probe(uint8_t addr) {
  _addr = addr;
  Wire.beginTransmission(_addr);
  if (Wire.endTransmission() != 0) {
    return false;
  }
  if (!write_cmd(kPowerOn) || !write_cmd(kContHres)) {
    return false;
  }
  delay(180);
  return poll();
}

bool TrackedAls::begin(uint8_t addr) {
  _ok = false;
  _lux = -1.0f;
  if (probe(addr) || (addr == 0x23 && probe(0x5C))) {
    _ok = true;
    return true;
  }
  return false;
}

bool TrackedAls::poll() {
  if (Wire.requestFrom(static_cast<int>(_addr), 2) != 2) {
    _lux = -1.0f;
    return false;
  }
  const uint16_t raw = (static_cast<uint16_t>(Wire.read()) << 8) | static_cast<uint16_t>(Wire.read());
  _lux = raw / 1.2f;
  _ok = true;
  return true;
}
