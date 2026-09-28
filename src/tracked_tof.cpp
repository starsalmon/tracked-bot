#include "tracked_tof.h"

#include <Wire.h>
#include <VL53L0X.h>

#include "tracked_pins.h"

namespace {
VL53L0X sensor;
}

bool TrackedTof::begin() {
  _ok = false;
  _present = false;
  _range_m = -1.0f;

  Wire.beginTransmission(TOF_I2C_ADDR);
  if (Wire.endTransmission() != 0) {
    return false;
  }
  _present = true;

  Wire.beginTransmission(TOF_I2C_ADDR);
  Wire.write(0xC0);
  if (Wire.endTransmission() != 0) {
    return false;
  }
  if (Wire.requestFrom(static_cast<uint8_t>(TOF_I2C_ADDR), static_cast<uint8_t>(1)) != 1) {
    return false;
  }
  const uint8_t model = static_cast<uint8_t>(Wire.read());
  // VL53L0X model id is 0xEE. L1X sits at 0x29 too but is a different driver.
  if (model != 0xEE) {
    Serial.printf("ToF at 0x29 id=0x%02X (need VL53L0X 0xEE)\n", model);
    return false;
  }

  sensor.setTimeout(50);
  if (!sensor.init()) {
    Serial.println("VL53L0X init failed");
    return false;
  }
  sensor.setMeasurementTimingBudget(33000);
  sensor.startContinuous();
  _ok = true;
  Serial.println("VL53L0X continuous");
  return true;
}

void TrackedTof::poll() {
  if (!_ok) {
    return;
  }
  const uint16_t mm = sensor.readRangeContinuousMillimeters();
  if (sensor.timeoutOccurred() || mm < 20 || mm > 2000) {
    _range_m = -1.0f;
    return;
  }
  _range_m = mm * 0.001f;
}
