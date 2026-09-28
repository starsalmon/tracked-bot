#include "tracked_imu.h"

#include <Wire.h>

#include "tracked_pins.h"

namespace {

constexpr uint8_t kRegWhoAmI = 0x75;
constexpr uint8_t kRegPwrMgmt1 = 0x6B;
constexpr uint8_t kRegGyroCfg = 0x1B;
constexpr uint8_t kRegAccelCfg = 0x1C;
constexpr uint8_t kRegData = 0x3B;
constexpr float kAccelLsbPerG = 16384.0f;
constexpr float kGyroLsbPerDps = 131.0f;
constexpr float kDegToRad = 0.01745329252f;

int16_t join16(uint8_t hi, uint8_t lo) {
  return static_cast<int16_t>((static_cast<uint16_t>(hi) << 8) | lo);
}

}  // namespace

bool Mpu6050Imu::write_byte(uint8_t reg, uint8_t value) const {
  Wire.beginTransmission(_addr);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool Mpu6050Imu::read_bytes(uint8_t reg, uint8_t* buf, size_t len) const {
  Wire.beginTransmission(_addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }
  const size_t got = Wire.requestFrom(static_cast<int>(_addr), static_cast<int>(len));
  if (got != len) {
    return false;
  }
  for (size_t i = 0; i < len; ++i) {
    buf[i] = static_cast<uint8_t>(Wire.read());
  }
  return true;
}

bool Mpu6050Imu::wake(uint8_t addr) {
  _addr = addr;
  if (!write_byte(kRegPwrMgmt1, 0x00)) {
    return false;
  }
  if (!write_byte(kRegGyroCfg, 0x00)) {
    return false;
  }
  if (!write_byte(kRegAccelCfg, 0x00)) {
    return false;
  }
  uint8_t who = 0;
  if (!read_bytes(kRegWhoAmI, &who, 1) || who != 0x68) {
    return false;
  }
  return true;
}

bool Mpu6050Imu::begin(uint8_t addr) {
  _ok = false;
  Wire.setClock(100000);
  delay(50);
  if (wake(addr) || (addr == 0x68 && wake(0x69))) {
    _ok = true;
    Wire.setClock(400000);
    ImuSample sample;
    return read(sample);
  }
  return false;
}

bool Mpu6050Imu::read(ImuSample& out) {
  if (!_ok) {
    return false;
  }
  uint8_t raw[14];
  if (!read_bytes(kRegData, raw, sizeof(raw))) {
    return false;
  }
  const int16_t ax = join16(raw[0], raw[1]);
  const int16_t ay = join16(raw[2], raw[3]);
  const int16_t az = join16(raw[4], raw[5]);
  const int16_t gx = join16(raw[8], raw[9]);
  const int16_t gy = join16(raw[10], raw[11]);
  const int16_t gz = join16(raw[12], raw[13]);
  out.ax = (static_cast<float>(ax) / kAccelLsbPerG) * 9.80665f;
  out.ay = (static_cast<float>(ay) / kAccelLsbPerG) * 9.80665f;
  out.az = (static_cast<float>(az) / kAccelLsbPerG) * 9.80665f;
  out.gx = static_cast<float>(gx) / kGyroLsbPerDps * kDegToRad;
  out.gy = static_cast<float>(gy) / kGyroLsbPerDps * kDegToRad;
  out.gz = static_cast<float>(gz) / kGyroLsbPerDps * kDegToRad * IMU_GYRO_Z_SIGN;
  return true;
}
